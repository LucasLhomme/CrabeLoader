/*
** CrabeLoader
** File description:
** WinHttpRedirector implementation
*/

#include "infrastructure/multiplayer/WinHttpRedirector.hpp"
#include "infrastructure/multiplayer/FallbackResponseProvider.hpp"

#include <algorithm>
#include <array>
#include <unordered_map>
#include "shared/logger.hpp"

#pragma comment(lib, "winhttp.lib")

namespace Multiplayer::Infrastructure {

    namespace {
        constexpr DWORD kWinHttpSecurityFlags =
            SECURITY_FLAG_IGNORE_UNKNOWN_CA |
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
            SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
            SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

        constexpr std::array<const wchar_t*, 12> kDisneyOldHosts = {
            L"disney.go.com", L"toys.disney.go.com", L"login.disney.go.com",
            L"auth.disney.go.com", L"matchmaking.disney.go.com", L"multiplayer.disney.go.com",
            L"cdn.disney.go.com", L"assets.disney.go.com", L"api.disney.com",
            L"ugc.disney.go.com", L"api.toybox.com", L"toybox.disney.go.com"
        };

        using WinHttpConnect_t = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
        using WinHttpOpenRequest_t = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
        using WinHttpSendRequest_t = BOOL(WINAPI*)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
        using WinHttpReceiveResponse_t = BOOL(WINAPI*)(HINTERNET, LPVOID);
        using WinHttpQueryDataAvailable_t = BOOL(WINAPI*)(HINTERNET, LPDWORD);
        using WinHttpReadData_t = BOOL(WINAPI*)(HINTERNET, LPVOID, DWORD, LPDWORD);
        using WinHttpCloseHandle_t = BOOL(WINAPI*)(HINTERNET);

        static std::unordered_map<HINTERNET, WinHttpRedirector::RequestContext> s_requests;
        static std::unordered_map<HINTERNET, std::wstring> s_requestObjects;
    } // namespace

    WinHttpRedirector::WinHttpRedirector() {
        if (!s_fallbackProvider) {
            s_fallbackProvider = std::make_unique<FallbackResponseProvider>();
        }
    }

    WinHttpRedirector::~WinHttpRedirector() {
        uninstall();
    }

    bool WinHttpRedirector::shouldRedirectHost(LPCWSTR serverName) {
        if (!serverName || serverName[0] == L'\0') {
            return false;
        }
        for (const wchar_t* oldHost : kDisneyOldHosts) {
            if (_wcsicmp(serverName, oldHost) == 0) return true;
        }
        return wcsstr(serverName, L"disney.com") ||
               wcsstr(serverName, L"disney.go.com") ||
               wcsstr(serverName, L"toybox.com");
    }

    void WinHttpRedirector::setDirectConnectPayload(std::string_view friendName, std::string_view locationString) {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_fallbackProvider) {
            s_fallbackProvider->setDirectConnectPayload(friendName, locationString);
        }
    }

    void WinHttpRedirector::clearDirectConnectPayload() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_fallbackProvider) {
            s_fallbackProvider->clearDirectConnectPayload();
        }
    }

    HINTERNET WINAPI WinHttpRedirector::HookedWinHttpConnect(
        HINTERNET hSession, LPCWSTR pswzServerName, INTERNET_PORT nServerPort, DWORD dwReserved)
    {
        LPCWSTR targetServer = pswzServerName;
        INTERNET_PORT targetPort = nServerPort;

        if (shouldRedirectHost(pswzServerName)) {
            std::lock_guard<std::mutex> lock(s_mutex);
            targetServer = s_targetHost.c_str();
            targetPort = s_targetPort;
            Logger::getInstance().debug("WinHttpRedirector: redirected Connect to 127.0.0.1:{}", targetPort);
        }

        auto orig = reinterpret_cast<WinHttpConnect_t>(s_connectHook.getOriginal());
        return orig ? orig(hSession, targetServer, targetPort, dwReserved)
                    : WinHttpConnect(hSession, targetServer, targetPort, dwReserved);
    }

    HINTERNET WINAPI WinHttpRedirector::HookedWinHttpOpenRequest(
        HINTERNET hConnect, LPCWSTR pwszVerb, LPCWSTR pwszObjectName, LPCWSTR pwszVersion,
        LPCWSTR pwszReferrer, LPCWSTR* ppwszAcceptTypes, DWORD dwFlags)
    {
        auto orig = reinterpret_cast<WinHttpOpenRequest_t>(s_openRequestHook.getOriginal());
        HINTERNET hRequest = orig ? orig(hConnect, pwszVerb, pwszObjectName, pwszVersion, pwszReferrer, ppwszAcceptTypes, dwFlags)
                                  : WinHttpOpenRequest(hConnect, pwszVerb, pwszObjectName, pwszVersion, pwszReferrer, ppwszAcceptTypes, dwFlags);

        if (hRequest) {
            DWORD secFlags = kWinHttpSecurityFlags;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
            if (pwszObjectName) {
                std::lock_guard<std::mutex> lock(s_mutex);
                s_requestObjects[hRequest] = pwszObjectName;
            }
        }
        return hRequest;
    }

    BOOL WINAPI WinHttpRedirector::HookedWinHttpSendRequest(
        HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength,
        LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext)
    {
        if (hRequest) {
            DWORD secFlags = kWinHttpSecurityFlags;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
        }

        auto orig = reinterpret_cast<WinHttpSendRequest_t>(s_sendRequestHook.getOriginal());
        BOOL ok = orig ? orig(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext)
                       : WinHttpSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);

        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_requestObjects.find(hRequest);
        if (it != s_requestObjects.end()) {
            bool hasPayload = s_fallbackProvider && s_fallbackProvider->hasDirectConnectPayload();
            bool forceDirect = hasPayload && (it->second.find(L"friends") != std::wstring::npos);
            if (!ok || forceDirect) {
                RequestContext ctx{ .bypassRead = true,
                                    .responseBody = s_fallbackProvider ? s_fallbackProvider->buildResponse(it->second) : "{\"status\":0}",
                                    .responseOffset = 0 };
                s_requests[hRequest] = std::move(ctx);
                return TRUE;
            }
        }
        return ok;
    }

    BOOL WINAPI WinHttpRedirector::HookedWinHttpReceiveResponse(HINTERNET hRequest, LPVOID lpReserved) {
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            auto it = s_requests.find(hRequest);
            if (it != s_requests.end() && it->second.bypassRead) return TRUE;
        }

        auto orig = reinterpret_cast<WinHttpReceiveResponse_t>(s_receiveResponseHook.getOriginal());
        BOOL res = orig ? orig(hRequest, lpReserved) : WinHttpReceiveResponse(hRequest, lpReserved);
        if (!res) {
            std::lock_guard<std::mutex> lock(s_mutex);
            auto itObj = s_requestObjects.find(hRequest);
            if (itObj != s_requestObjects.end()) {
                RequestContext ctx{ .bypassRead = true,
                                    .responseBody = s_fallbackProvider ? s_fallbackProvider->buildResponse(itObj->second) : "{\"status\":0}",
                                    .responseOffset = 0 };
                s_requests[hRequest] = std::move(ctx);
                return TRUE;
            }
        }
        return res;
    }

    BOOL WINAPI WinHttpRedirector::HookedWinHttpQueryDataAvailable(HINTERNET hRequest, LPDWORD lpdwNumberOfBytesAvailable) {
        if (!lpdwNumberOfBytesAvailable) return FALSE;
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_requests.find(hRequest);
        if (it != s_requests.end() && it->second.bypassRead) {
            size_t remaining = it->second.responseBody.size() - it->second.responseOffset;
            *lpdwNumberOfBytesAvailable = static_cast<DWORD>(remaining);
            return TRUE;
        }

        auto orig = reinterpret_cast<WinHttpQueryDataAvailable_t>(s_queryDataHook.getOriginal());
        return orig ? orig(hRequest, lpdwNumberOfBytesAvailable) : WinHttpQueryDataAvailable(hRequest, lpdwNumberOfBytesAvailable);
    }

    BOOL WINAPI WinHttpRedirector::HookedWinHttpReadData(
        HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead)
    {
        if (!lpBuffer || !lpdwNumberOfBytesRead) return FALSE;
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_requests.find(hRequest);
        if (it != s_requests.end() && it->second.bypassRead) {
            size_t remaining = it->second.responseBody.size() - it->second.responseOffset;
            DWORD toCopy = static_cast<DWORD>(std::min(static_cast<size_t>(dwNumberOfBytesToRead), remaining));
            if (toCopy > 0) {
                std::memcpy(lpBuffer, it->second.responseBody.data() + it->second.responseOffset, toCopy);
                it->second.responseOffset += toCopy;
            }
            *lpdwNumberOfBytesRead = toCopy;
            return TRUE;
        }

        auto orig = reinterpret_cast<WinHttpReadData_t>(s_readDataHook.getOriginal());
        return orig ? orig(hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead)
                    : WinHttpReadData(hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
    }

    BOOL WINAPI WinHttpRedirector::HookedWinHttpCloseHandle(HINTERNET hInternet) {
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_requests.erase(hInternet);
            s_requestObjects.erase(hInternet);
        }

        auto orig = reinterpret_cast<WinHttpCloseHandle_t>(s_closeHandleHook.getOriginal());
        return orig ? orig(hInternet) : WinHttpCloseHandle(hInternet);
    }

    bool WinHttpRedirector::installDetours(HMODULE hWinHttp) {
        auto pConnect = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpConnect"));
        auto pOpen = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpOpenRequest"));
        auto pSend = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpSendRequest"));
        auto pRecv = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpReceiveResponse"));
        auto pQuery = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpQueryDataAvailable"));
        auto pRead = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpReadData"));
        auto pClose = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpCloseHandle"));

        if (!pConnect || !pOpen || !pSend || !pRecv || !pQuery || !pRead || !pClose) {
            return false;
        }

        return s_connectHook.install(pConnect, reinterpret_cast<void*>(&HookedWinHttpConnect)) &&
               s_openRequestHook.install(pOpen, reinterpret_cast<void*>(&HookedWinHttpOpenRequest)) &&
               s_sendRequestHook.install(pSend, reinterpret_cast<void*>(&HookedWinHttpSendRequest)) &&
               s_receiveResponseHook.install(pRecv, reinterpret_cast<void*>(&HookedWinHttpReceiveResponse)) &&
               s_queryDataHook.install(pQuery, reinterpret_cast<void*>(&HookedWinHttpQueryDataAvailable)) &&
               s_readDataHook.install(pRead, reinterpret_cast<void*>(&HookedWinHttpReadData)) &&
               s_closeHandleHook.install(pClose, reinterpret_cast<void*>(&HookedWinHttpCloseHandle));
    }

    std::expected<void, std::string> WinHttpRedirector::install() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_installed.load()) return {};

        HMODULE hWinHttp = GetModuleHandleW(L"winhttp.dll");
        if (!hWinHttp) hWinHttp = LoadLibraryW(L"winhttp.dll");
        if (!hWinHttp) return std::unexpected("winhttp.dll not found in process");

        if (!installDetours(hWinHttp)) {
            uninstall();
            return std::unexpected("Failed to install WinHttp detours");
        }

        s_installed = true;
        Logger::getInstance().info("WinHttpRedirector: hooked all 7 WinHttp APIs -> 127.0.0.1:{}", s_targetPort);
        return {};
    }

    void WinHttpRedirector::uninstall() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_installed.load()) return;

        s_closeHandleHook.remove();
        s_readDataHook.remove();
        s_queryDataHook.remove();
        s_receiveResponseHook.remove();
        s_sendRequestHook.remove();
        s_openRequestHook.remove();
        s_connectHook.remove();
        s_requests.clear();
        s_requestObjects.clear();
        s_installed = false;
        Logger::getInstance().info("WinHttpRedirector: unhooked WinHttp APIs");
    }

    void WinHttpRedirector::setTarget(std::wstring_view host, uint16_t port) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_targetHost = host;
        s_targetPort = port;
    }

    std::wstring_view WinHttpRedirector::getTargetHost() const noexcept {
        return s_targetHost;
    }

    uint16_t WinHttpRedirector::getTargetPort() const noexcept {
        return s_targetPort;
    }

    bool WinHttpRedirector::isInstalled() const noexcept {
        return s_installed.load();
    }

} // namespace Multiplayer::Infrastructure
