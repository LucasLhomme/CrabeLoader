/*
** CrabeLoader
** File description:
** WinHttpRedirector - MinHook redirection of WinHttp API for Disney Infinity
*/

#ifndef WINHTTP_REDIRECTOR_HPP_
#define WINHTTP_REDIRECTOR_HPP_

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <windows.h>
#include <winhttp.h>

#include "infrastructure/hook.hpp"
#include "domain/multiplayer/i_fallback_response_provider.hpp"
#include "domain/multiplayer/i_network_redirector.hpp"

namespace crabe::multiplayer::infrastructure {

    class WinHttpRedirector final : public domain::INetworkRedirector {
    public:
        WinHttpRedirector();
        ~WinHttpRedirector() override;

        WinHttpRedirector(const WinHttpRedirector&) = delete;
        WinHttpRedirector& operator=(const WinHttpRedirector&) = delete;
        WinHttpRedirector(WinHttpRedirector&&) = delete;
        WinHttpRedirector& operator=(WinHttpRedirector&&) = delete;

        [[nodiscard]] std::expected<void, std::string> install() override;
        void uninstall() override;

        void setTarget(std::wstring_view host, uint16_t port) override;
        [[nodiscard]] std::wstring_view getTargetHost() const noexcept override;
        [[nodiscard]] uint16_t getTargetPort() const noexcept override;

        [[nodiscard]] bool isInstalled() const noexcept override;

        void setDirectConnectPayload(std::string_view friendName, std::string_view locationString);
        void clearDirectConnectPayload();

        // Static trampoline accessors for MinHook detours
        static HINTERNET WINAPI HookedWinHttpConnect(
            HINTERNET hSession,
            LPCWSTR pswzServerName,
            INTERNET_PORT nServerPort,
            DWORD dwReserved);

        static HINTERNET WINAPI HookedWinHttpOpenRequest(
            HINTERNET hConnect,
            LPCWSTR pwszVerb,
            LPCWSTR pwszObjectName,
            LPCWSTR pwszVersion,
            LPCWSTR pwszReferrer,
            LPCWSTR* ppwszAcceptTypes,
            DWORD dwFlags);

        static BOOL WINAPI HookedWinHttpSendRequest(
            HINTERNET hRequest,
            LPCWSTR lpszHeaders,
            DWORD dwHeadersLength,
            LPVOID lpOptional,
            DWORD dwOptionalLength,
            DWORD dwTotalLength,
            DWORD_PTR dwContext);

        static BOOL WINAPI HookedWinHttpReceiveResponse(
            HINTERNET hRequest,
            LPVOID lpReserved);

        static BOOL WINAPI HookedWinHttpQueryDataAvailable(
            HINTERNET hRequest,
            LPDWORD lpdwNumberOfBytesAvailable);

        static BOOL WINAPI HookedWinHttpReadData(
            HINTERNET hRequest,
            LPVOID lpBuffer,
            DWORD dwNumberOfBytesToRead,
            LPDWORD lpdwNumberOfBytesRead);

        static BOOL WINAPI HookedWinHttpCloseHandle(
            HINTERNET hInternet);

        struct RequestContext {
            bool bypassRead{ false };
            std::string responseBody;
            size_t responseOffset{ 0 };
        };

    private:

        static bool shouldRedirectHost(LPCWSTR serverName);
        static bool installDetours(HMODULE hWinHttp);

        static inline std::mutex s_mutex;
        static inline std::wstring s_targetHost{ L"127.0.0.1" };
        static inline uint16_t s_targetPort{ 3000 };
        static inline std::atomic<bool> s_installed{ false };

        static inline std::unique_ptr<domain::IFallbackResponseProvider> s_fallbackProvider;

        static inline crabe::infrastructure::Hook s_connectHook;
        static inline crabe::infrastructure::Hook s_openRequestHook;
        static inline crabe::infrastructure::Hook s_sendRequestHook;
        static inline crabe::infrastructure::Hook s_receiveResponseHook;
        static inline crabe::infrastructure::Hook s_queryDataHook;
        static inline crabe::infrastructure::Hook s_readDataHook;
        static inline crabe::infrastructure::Hook s_closeHandleHook;
    };

} // namespace crabe::multiplayer::infrastructure

#endif /* !WINHTTP_REDIRECTOR_HPP_ */

