/*
** CrabeLoader
** File description:
** Portal transports and device discovery:
**  - HID (PS3 / PS4 / Wii / Wii U / PC bases, VID 0x0E6F PID 0x0129): report id 0 + 32 bytes.
**  - Xbox 360 base (VID 0x24C6 PID 0xFA00) through WinUSB: 32-byte packet [0x0B, 0x15, frame].
**  - Xbox One base: see portal_transport_gip.cpp.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_transport.hpp"
#include "infrastructure/portal/portal_transport_factory.hpp"
#include "infrastructure/portal/usb_device.hpp"

#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>

#include <algorithm>
#include <cwctype>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace crabe::infrastructure::portal {

namespace {

constexpr std::uint16_t kVidPdp = 0x0E6F;
constexpr std::uint16_t kPidStandardHid = 0x0129;
constexpr std::uint16_t kPidXboxOne = 0x0229;
constexpr std::uint16_t kVidXbox360 = 0x24C6;
constexpr std::uint16_t kPidXbox360 = 0xFA00;

constexpr unsigned kWriteTimeoutMs = 500;

// ------------------------------------------------------------------- HID
class HidTransport final : public IPortalTransport {
public:
    explicit HidTransport(std::wstring path) : m_path(std::move(path)) {}
    ~HidTransport() override { Close(); }

    bool Open() override {
        Close();
        m_device = CreateFileW(m_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (m_device == INVALID_HANDLE_VALUE) {
            m_diagnostic = "Portail USB HID détecté mais impossible de l'ouvrir (déjà utilisé par un autre programme ?).";
            return false;
        }

        PHIDP_PREPARSED_DATA preparsed = nullptr;
        HIDP_CAPS caps{};
        const bool haveCaps = HidD_GetPreparsedData(m_device, &preparsed) &&
                              HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS;
        if (preparsed) HidD_FreePreparsedData(preparsed);
        if (!haveCaps || caps.InputReportByteLength < kReportSize + 1 || caps.OutputReportByteLength < kReportSize + 1) {
            m_diagnostic = "Le périphérique HID n'a pas les rapports de 32 octets d'un portail Infinity.";
            Close();
            return false;
        }
        m_inLen = caps.InputReportByteLength;
        m_outLen = caps.OutputReportByteLength;
        m_readBuffer.assign(m_inLen, 0);

        m_readEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        m_writeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        return m_readEvent && m_writeEvent;
    }

    void Close() override {
        if (m_device != INVALID_HANDLE_VALUE && m_readPending) {
            CancelIoEx(m_device, &m_readOverlapped);
            DWORD ignored = 0;
            GetOverlappedResult(m_device, &m_readOverlapped, &ignored, TRUE);
        }
        m_readPending = false;
        if (m_device != INVALID_HANDLE_VALUE) CloseHandle(m_device);
        m_device = INVALID_HANDLE_VALUE;
        if (m_readEvent) CloseHandle(m_readEvent);
        if (m_writeEvent) CloseHandle(m_writeEvent);
        m_readEvent = m_writeEvent = nullptr;
    }

    bool Write(const Report& report) override {
        if (m_device == INVALID_HANDLE_VALUE) return false;
        std::vector<std::uint8_t> out(m_outLen, 0);
        std::copy(report.begin(), report.end(), out.begin() + 1); // byte 0 is the HID report id

        OVERLAPPED ov{};
        ov.hEvent = m_writeEvent;
        ResetEvent(m_writeEvent);
        DWORD written = 0;
        if (!WriteFile(m_device, out.data(), static_cast<DWORD>(out.size()), &written, &ov)) {
            if (GetLastError() != ERROR_IO_PENDING) return false;
            if (WaitForSingleObject(m_writeEvent, kWriteTimeoutMs) != WAIT_OBJECT_0) {
                CancelIoEx(m_device, &ov);
                GetOverlappedResult(m_device, &ov, &written, TRUE);
                return false;
            }
            if (!GetOverlappedResult(m_device, &ov, &written, FALSE)) return false;
        }
        return written > 0;
    }

    ReadResult Read(Report& report, unsigned timeoutMs) override {
        if (m_device == INVALID_HANDLE_VALUE) return ReadResult::Lost;

        if (!m_readPending) {
            m_readOverlapped = {};
            m_readOverlapped.hEvent = m_readEvent;
            ResetEvent(m_readEvent);
            std::fill(m_readBuffer.begin(), m_readBuffer.end(), std::uint8_t{0});
            DWORD ignored = 0;
            if (!ReadFile(m_device, m_readBuffer.data(), static_cast<DWORD>(m_readBuffer.size()), &ignored, &m_readOverlapped) &&
                GetLastError() != ERROR_IO_PENDING) {
                return ReadResult::Lost;
            }
            m_readPending = true;
        }

        if (WaitForSingleObject(m_readEvent, timeoutMs) != WAIT_OBJECT_0) return ReadResult::Timeout;

        DWORD received = 0;
        m_readPending = false;
        if (!GetOverlappedResult(m_device, &m_readOverlapped, &received, FALSE)) return ReadResult::Lost;
        if (received < kReportSize + 1) return ReadResult::Timeout;

        std::copy_n(m_readBuffer.begin() + 1, kReportSize, report.begin());
        return ReadResult::Ok;
    }

    PortalType Type() const override { return PortalType::StandardHid; }
    std::string Description() const override { return "Portail USB HID (PS3 / PS4 / Wii / Wii U / PC)"; }
    std::string Diagnostic() const override { return m_diagnostic; }

private:
    std::wstring m_path;
    HANDLE m_device{INVALID_HANDLE_VALUE};
    HANDLE m_readEvent{nullptr};
    HANDLE m_writeEvent{nullptr};
    OVERLAPPED m_readOverlapped{};
    std::vector<std::uint8_t> m_readBuffer;
    bool m_readPending{false};
    std::size_t m_inLen{0};
    std::size_t m_outLen{0};
    std::string m_diagnostic;
};

// ------------------------------------------------------------- Xbox 360
class Xbox360Transport final : public IPortalTransport {
public:
    bool Open() override {
        if (!m_usb.Open(kVidXbox360, kPidXbox360)) {
            m_diagnostic = "Portail Xbox 360 détecté mais impossible de l'ouvrir via WinUSB.";
            return false;
        }
        return true;
    }
    void Close() override { m_usb.Close(); }

    bool Write(const Report& report) override {
        // The 360 link carries the 32-byte report with a two-byte header, which leaves 30 bytes
        // for the Disney frame; no request the protocol builds is longer than that.
        std::array<std::uint8_t, kReportSize> packet{};
        packet[0] = 0x0B;
        packet[1] = 0x15;
        std::copy_n(report.begin(), kReportSize - 2, packet.begin() + 2);
        return m_usb.Write(packet, kWriteTimeoutMs) == UsbDevice::IoResult::Ok;
    }

    ReadResult Read(Report& report, unsigned timeoutMs) override {
        std::array<std::uint8_t, 64> packet{};
        std::size_t received = 0;
        switch (m_usb.Read(packet, timeoutMs, received)) {
            case UsbDevice::IoResult::Timeout: return ReadResult::Timeout;
            case UsbDevice::IoResult::Lost: return ReadResult::Lost;
            case UsbDevice::IoResult::Ok: break;
        }
        if (received < 3) return ReadResult::Timeout;
        report.fill(0);
        const std::size_t count = std::min<std::size_t>(received - 2, kReportSize);
        std::copy_n(packet.begin() + 2, count, report.begin());
        return ReadResult::Ok;
    }

    PortalType Type() const override { return PortalType::Xbox360; }
    std::string Description() const override { return "Portail Xbox 360 (WinUSB)"; }
    std::string Diagnostic() const override { return m_diagnostic; }

private:
    UsbDevice m_usb;
    std::string m_diagnostic;
};

std::wstring Lower(std::wstring text) {
    for (auto& c : text) c = static_cast<wchar_t>(std::towlower(c));
    return text;
}

// Device paths of every HID interface of the given VID/PID.
std::vector<std::wstring> FindHidPaths(std::uint16_t vid, std::uint16_t pid) {
    std::vector<std::wstring> found;
    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);
    HDEVINFO info = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) return found;

    wchar_t token[32];
    swprintf_s(token, L"vid_%04x&pid_%04x", vid, pid);

    SP_DEVICE_INTERFACE_DATA data{};
    data.cbSize = sizeof(data);
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(info, nullptr, &hidGuid, index, &data); ++index) {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetailW(info, &data, nullptr, 0, &needed, nullptr);
        if (needed < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;
        std::vector<std::uint8_t> storage(needed);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (SetupDiGetDeviceInterfaceDetailW(info, &data, detail, needed, nullptr, nullptr) &&
            Lower(detail->DevicePath).find(token) != std::wstring::npos) {
            found.emplace_back(detail->DevicePath);
        }
    }
    SetupDiDestroyDeviceInfoList(info);
    return found;
}

} // namespace

DiscoveryResult DiscoverTransports() {
    DiscoveryResult result;

    for (auto& path : FindHidPaths(kVidPdp, kPidStandardHid)) {
        result.transports.push_back(std::make_unique<HidTransport>(std::move(path)));
    }

    if (UsbDevice::IsPresent(kVidXbox360, kPidXbox360)) {
        if (UsbDevice::HasWinUsbDriver(kVidXbox360, kPidXbox360)) {
            result.transports.push_back(std::make_unique<Xbox360Transport>());
        } else {
            result.diagnostic = "Portail Xbox 360 détecté mais sans pilote WinUSB : installez-le avec Zadig (voir le README).";
        }
    }

    if (UsbDevice::IsPresent(kVidPdp, kPidXboxOne)) {
        // The inbox Xbox driver does not expose the base to applications: WinUSB is required.
        if (UsbDevice::HasWinUsbDriver(kVidPdp, kPidXboxOne)) {
            result.transports.push_back(MakeXboxOneTransport());
        } else {
            result.diagnostic = "Portail Xbox One détecté mais sans pilote WinUSB : installez-le avec Zadig (voir le README).";
        }
    }
    return result;
}

} // namespace crabe::infrastructure::portal
