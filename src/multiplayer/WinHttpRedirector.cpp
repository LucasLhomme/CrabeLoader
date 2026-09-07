/*
** CrabeLoader
** File description:
** WinHttpRedirector implementation
*/

#include "loader/multiplayer/infrastructure/WinHttpRedirector.hpp"

#include <algorithm>
#include <array>
#include "logger/logger.hpp"

#pragma comment(lib, "winhttp.lib")

namespace Multiplayer::Infrastructure {

    namespace {
        constexpr DWORD kWinHttpSecurityFlags =
            SECURITY_FLAG_IGNORE_UNKNOWN_CA |
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
            SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
            SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

        constexpr std::array<const wchar_t*, 4> kDisneyOldHosts = {
            L"api.disney.com",
            L"ugc.disney.go.com",
            L"api.toybox.com",
            L"toybox.disney.go.com"
        };

        using WinHttpConnect_t = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
        using WinHttpOpenRequest_t = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
        using WinHttpSendRequest_t = BOOL(WINAPI*)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
    } // namespace

    WinHttpRedirector::WinHttpRedirector() = default;

    WinHttpRedirector::~WinHttpRedirector() {
        uninstall();
    }

    bool WinHttpRedirector::shouldRedirectHost(LPCWSTR serverName) {
        if (!serverName || serverName[0] == L'\0') {
            return false;
        }
        for (const wchar_t* oldHost : kDisneyOldHosts) {
            if (_wcsicmp(serverName, oldHost) == 0) {
                return true;
            }
        }
        return false;
    }

    HINTERNET WINAPI WinHttpRedirector::HookedWinHttpConnect(
        HINTERNET hSession,
        LPCWSTR pswzServerName,
        INTERNET_PORT nServerPort,
        DWORD dwReserved)
    {
        LPCWSTR targetServer = pswzServerName;
        INTERNET_PORT targetPort = nServerPort;

        if (shouldRedirectHost(pswzServerName)) {
            std::lock_guard<std::mutex> lock(s_mutex);
            targetServer = s_targetHost.c_str();
            targetPort = s_targetPort;
            Logger::getInstance().debug("WinHttpRedirector: redirected Connect from '{}' to '{}:{}'",
                                        pswzServerName ? "disney" : "unknown",
                                        "127.0.0.1", targetPort);
        }

        auto orig = reinterpret_cast<WinHttpConnect_t>(s_connectHook.getOriginal());
        if (!orig) {
            return WinHttpConnect(hSession, targetServer, targetPort, dwReserved);
        }
        return orig(hSession, targetServer, targetPort, dwReserved);
    }

    HINTERNET WINAPI WinHttpRedirector::HookedWinHttpOpenRequest(
        HINTERNET hConnect,
        LPCWSTR pwszVerb,
        LPCWSTR pwszObjectName,
        LPCWSTR pwszVersion,
        LPCWSTR pwszReferrer,
        LPCWSTR* ppwszAcceptTypes,
        DWORD dwFlags)
    {
        auto orig = reinterpret_cast<WinHttpOpenRequest_t>(s_openRequestHook.getOriginal());
        HINTERNET hRequest = orig ? orig(hConnect, pwszVerb, pwszObjectName, pwszVersion, pwszReferrer, ppwszAcceptTypes, dwFlags)
                                  : WinHttpOpenRequest(hConnect, pwszVerb, pwszObjectName, pwszVersion, pwszReferrer, ppwszAcceptTypes, dwFlags);

        if (hRequest) {
            DWORD secFlags = kWinHttpSecurityFlags;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
        }

        return hRequest;
    }

    BOOL WINAPI WinHttpRedirector::HookedWinHttpSendRequest(
        HINTERNET hRequest,
        LPCWSTR lpszHeaders,
        DWORD dwHeadersLength,
        LPVOID lpOptional,
        DWORD dwOptionalLength,
        DWORD dwTotalLength,
        DWORD_PTR dwContext)
    {
        if (hRequest) {
            DWORD secFlags = kWinHttpSecurityFlags;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
        }

        auto orig = reinterpret_cast<WinHttpSendRequest_t>(s_sendRequestHook.getOriginal());
        if (!orig) {
            return WinHttpSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);
        }
        return orig(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);
    }

    std::expected<void, std::string> WinHttpRedirector::install() {
        std::lock_guard<std::mutex> lock(s_mutex);

        if (s_installed.load()) {
            return {};
        }

        HMODULE hWinHttp = GetModuleHandleW(L"winhttp.dll");
        if (!hWinHttp) {
            hWinHttp = LoadLibraryW(L"winhttp.dll");
        }
        if (!hWinHttp) {
            return std::unexpected("winhttp.dll not found in process");
        }

        void* pConnect = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpConnect"));
        void* pOpenRequest = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpOpenRequest"));
        void* pSendRequest = reinterpret_cast<void*>(GetProcAddress(hWinHttp, "WinHttpSendRequest"));

        if (!pConnect || !pOpenRequest || !pSendRequest) {
            return std::unexpected("Failed to resolve WinHttp functions");
        }

        if (!s_connectHook.install(pConnect, reinterpret_cast<void*>(&HookedWinHttpConnect))) {
            return std::unexpected("Failed to hook WinHttpConnect");
        }

        if (!s_openRequestHook.install(pOpenRequest, reinterpret_cast<void*>(&HookedWinHttpOpenRequest))) {
            s_connectHook.remove();
            return std::unexpected("Failed to hook WinHttpOpenRequest");
        }

        if (!s_sendRequestHook.install(pSendRequest, reinterpret_cast<void*>(&HookedWinHttpSendRequest))) {
            s_openRequestHook.remove();
            s_connectHook.remove();
            return std::unexpected("Failed to hook WinHttpSendRequest");
        }

        s_installed = true;
        Logger::getInstance().info("WinHttpRedirector: hooked WinHttp APIs -> {}:{}",
                                   "127.0.0.1", s_targetPort);
        return {};
    }

    void WinHttpRedirector::uninstall() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_installed.load()) {
            return;
        }

        s_sendRequestHook.remove();
        s_openRequestHook.remove();
        s_connectHook.remove();

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

