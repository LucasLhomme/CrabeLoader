#include "loader/multiplayer/infrastructure/UpnpNatService.hpp"
#include "logger/logger.hpp"

#include <windows.h>
#include <natupnp.h>
#include <comdef.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <array>
#include <random>
#include <sstream>

_COM_SMARTPTR_TYPEDEF(IUPnPNAT, __uuidof(IUPnPNAT));
_COM_SMARTPTR_TYPEDEF(IStaticPortMappingCollection, __uuidof(IStaticPortMappingCollection));
_COM_SMARTPTR_TYPEDEF(IStaticPortMapping, __uuidof(IStaticPortMapping));

namespace Crabe::Multiplayer {

    UpnpNatService::UpnpNatService()
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        _comInitialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

        WSADATA wsaData{};
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        _status.localIp = getLocalIpAddress();
        _status.statusMessage = "UPnP service initialized";
    }

    UpnpNatService::~UpnpNatService()
    {
        if (_hasActiveLease && _forwardedPort != 0) {
            (void)releasePort(_forwardedPort, _forwardedProtocol);
        }

        WSACleanup();

        if (_comInitialized) {
            CoUninitialize();
            _comInitialized = false;
        }
    }

    std::expected<NatStatus, std::string> UpnpNatService::forwardPort(
        uint16_t port,
        std::string_view protocol,
        std::string_view description
    )
    {
        std::lock_guard<std::mutex> lock(_mutex);
        Logger& logger = Logger::getInstance();

        logger.info("UpnpNatService: Requesting automatic port forwarding for port {}/{}...", port, protocol);

        _status.internalPort = port;
        _status.externalPort = port;
        _status.protocol = std::string(protocol);
        _status.localIp = getLocalIpAddress();

        IUPnPNATPtr nat = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_UPnPNAT,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IUPnPNAT,
            reinterpret_cast<void**>(&nat)
        );

        if (FAILED(hr) || !nat) {
            _status.upnpAvailable = false;
            _status.portForwarded = false;
            _status.statusMessage = "UPnP not supported or disabled on local network/router";
            logger.warning("UpnpNatService: Failed to instantiate IUPnPNAT (HRESULT: 0x{:08X})", static_cast<uint32_t>(hr));

            // Still resolve public IP via STUN so player has their external address
            _status.externalIp = resolveExternalIpViaStun();
            return _status;
        }

        _status.upnpAvailable = true;

        IStaticPortMappingCollectionPtr mappings = nullptr;
        hr = nat->get_StaticPortMappingCollection(&mappings);
        if (FAILED(hr) || !mappings) {
            _status.portForwarded = false;
            _status.statusMessage = "Router does not expose UPnP StaticPortMappingCollection (IGD disabled)";
            logger.warning("UpnpNatService: Failed to retrieve StaticPortMappingCollection (HRESULT: 0x{:08X})", static_cast<uint32_t>(hr));
            _status.externalIp = resolveExternalIpViaStun();
            return _status;
        }

        IStaticPortMappingPtr mapping = nullptr;
        _bstr_t bstrProtocol(protocol.data());
        _bstr_t bstrLocalIp(_status.localIp.c_str());
        _bstr_t bstrDesc(description.data());

        // Remove any stale mapping first
        (void)mappings->Remove(static_cast<long>(port), bstrProtocol);

        hr = mappings->Add(
            static_cast<long>(port),
            bstrProtocol,
            static_cast<long>(port),
            bstrLocalIp,
            VARIANT_TRUE,
            bstrDesc,
            &mapping
        );

        if (FAILED(hr)) {
            _status.portForwarded = false;
            _status.statusMessage = std::format("UPnP AddPortMapping failed (0x{:08X})", static_cast<uint32_t>(hr));
            logger.warning("UpnpNatService: mappings->Add failed for port {} (HRESULT: 0x{:08X})", port, static_cast<uint32_t>(hr));
            _status.externalIp = resolveExternalIpViaStun();
            return _status;
        }

        _status.portForwarded = true;
        _hasActiveLease = true;
        _forwardedPort = port;
        _forwardedProtocol = std::string(protocol);
        _status.statusMessage = std::format("Port {}/{} forwarded successfully via UPnP IGD", port, protocol);

        logger.info("UpnpNatService: Port {}/{} successfully forwarded to local IP {}", port, protocol, _status.localIp);

        // Try to query router's external IP from mapping
        if (mapping) {
            BSTR bstrExternal = nullptr;
            if (SUCCEEDED(mapping->get_ExternalIPAddress(&bstrExternal)) && bstrExternal) {
                _status.externalIp = _bstr_t(bstrExternal);
                SysFreeString(bstrExternal);
            }
        }

        if (_status.externalIp.empty() || _status.externalIp == "0.0.0.0") {
            _status.externalIp = resolveExternalIpViaStun();
        }

        logger.info("UpnpNatService: Discovered External Public IP: {}", _status.externalIp);
        return _status;
    }

    std::expected<void, std::string> UpnpNatService::releasePort(uint16_t port, std::string_view protocol)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        Logger& logger = Logger::getInstance();

        IUPnPNATPtr nat = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_UPnPNAT,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IUPnPNAT,
            reinterpret_cast<void**>(&nat)
        );

        if (SUCCEEDED(hr) && nat) {
            IStaticPortMappingCollectionPtr mappings = nullptr;
            if (SUCCEEDED(nat->get_StaticPortMappingCollection(&mappings)) && mappings) {
                _bstr_t bstrProtocol(protocol.data());
                mappings->Remove(static_cast<long>(port), bstrProtocol);
                logger.info("UpnpNatService: Released UPnP port lease for {}/{}", port, protocol);
            }
        }

        _hasActiveLease = false;
        _status.portForwarded = false;
        _status.statusMessage = std::format("Port lease {} released", port);
        return {};
    }

    NatStatus UpnpNatService::getStatus() const noexcept
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _status;
    }

    std::string UpnpNatService::getLocalIpAddress() const
    {
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            return "127.0.0.1";
        }

        sockaddr_in probe{};
        probe.sin_family = AF_INET;
        probe.sin_port = htons(53);
        inet_pton(AF_INET, "8.8.8.8", &probe.sin_addr);

        if (connect(sock, reinterpret_cast<sockaddr*>(&probe), sizeof(probe)) == SOCKET_ERROR) {
            closesocket(sock);
            return "127.0.0.1";
        }

        sockaddr_in local{};
        int localLen = sizeof(local);
        if (getsockname(sock, reinterpret_cast<sockaddr*>(&local), &localLen) == SOCKET_ERROR) {
            closesocket(sock);
            return "127.0.0.1";
        }

        closesocket(sock);

        char ipStr[INET_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET, &local.sin_addr, ipStr, sizeof(ipStr))) {
            return std::string(ipStr);
        }

        return "127.0.0.1";
    }

    std::string UpnpNatService::resolveExternalIpViaStun() const
    {
        Logger& logger = Logger::getInstance();

        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            return "";
        }

        // 1 second timeout
        DWORD timeoutMs = 1200;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        addrinfo* res = nullptr;
        if (getaddrinfo("stun.l.google.com", "19302", &hints, &res) != 0 || !res) {
            closesocket(sock);
            return "";
        }

        // STUN Binding Request (RFC 5389): 20 bytes
        // 0-1: Msg Type (0x0001)
        // 2-3: Msg Length (0x0000)
        // 4-7: Magic Cookie (0x2112A442)
        // 8-19: 12-byte random transaction ID
        std::array<uint8_t, 20> stunRequest{};
        stunRequest[0] = 0x00;
        stunRequest[1] = 0x01;
        stunRequest[2] = 0x00;
        stunRequest[3] = 0x00;
        stunRequest[4] = 0x21;
        stunRequest[5] = 0x12;
        stunRequest[6] = 0xA4;
        stunRequest[7] = 0x42;

        std::random_device rd;
        for (size_t i = 8; i < 20; ++i) {
            stunRequest[i] = static_cast<uint8_t>(rd() & 0xFF);
        }

        int sent = sendto(sock, reinterpret_cast<const char*>(stunRequest.data()), sizeof(stunRequest), 0, res->ai_addr, static_cast<int>(res->ai_addrlen));
        freeaddrinfo(res);

        if (sent <= 0) {
            closesocket(sock);
            return "";
        }

        std::array<uint8_t, 512> response{};
        int received = recv(sock, reinterpret_cast<char*>(response.data()), sizeof(response), 0);
        closesocket(sock);

        if (received < 20) {
            return "";
        }

        // Validate Binding Response (0x0101) and Magic Cookie
        uint16_t msgType = (response[0] << 8) | response[1];
        uint32_t cookie = (response[4] << 24) | (response[5] << 16) | (response[6] << 8) | response[7];

        if (msgType != 0x0101 || cookie != 0x2112A442) {
            return "";
        }

        // Parse attributes looking for XOR-MAPPED-ADDRESS (0x0020) or MAPPED-ADDRESS (0x0001)
        size_t offset = 20;
        while (offset + 4 <= static_cast<size_t>(received)) {
            uint16_t attrType = (response[offset] << 8) | response[offset + 1];
            uint16_t attrLen = (response[offset + 2] << 8) | response[offset + 3];
            offset += 4;

            if (offset + attrLen > static_cast<size_t>(received)) {
                break;
            }

            if (attrType == 0x0020 && attrLen >= 8) { // XOR-MAPPED-ADDRESS
                uint8_t family = response[offset + 1];
                if (family == 0x01) { // IPv4
                    uint32_t xorIp = (response[offset + 4] << 24) | (response[offset + 5] << 16) | (response[offset + 6] << 8) | response[offset + 7];
                    uint32_t realIp = xorIp ^ 0x2112A442;

                    in_addr addr{};
                    addr.s_addr = htonl(realIp);
                    char ipStr[INET_ADDRSTRLEN] = {0};
                    if (inet_ntop(AF_INET, &addr, ipStr, sizeof(ipStr))) {
                        logger.info("UpnpNatService: STUN resolved external IP: {}", ipStr);
                        return std::string(ipStr);
                    }
                }
            } else if (attrType == 0x0001 && attrLen >= 8) { // MAPPED-ADDRESS
                uint8_t family = response[offset + 1];
                if (family == 0x01) { // IPv4
                    in_addr addr{};
                    std::memcpy(&addr.s_addr, &response[offset + 4], 4);
                    char ipStr[INET_ADDRSTRLEN] = {0};
                    if (inet_ntop(AF_INET, &addr, ipStr, sizeof(ipStr))) {
                        logger.info("UpnpNatService: STUN resolved external IP: {}", ipStr);
                        return std::string(ipStr);
                    }
                }
            }

            offset += (attrLen + 3) & ~3; // 32-bit aligned
        }

        return "";
    }

} // namespace Crabe::Multiplayer
