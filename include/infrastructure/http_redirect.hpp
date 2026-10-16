/*
** CrabeLoader
** File description:
** Declares the HTTP redirect: routes WinHTTP connections for configured servers to another host.
** The rules come from crabe.toml [http.redirect]; hooks WinHttpConnect and OpenRequest.
** Knows no server names of its own; request paths, headers and bodies are untouched.
**
** Authors: @LucasLhomme
*/

#ifndef HTTP_REDIRECT_HPP_
#define HTTP_REDIRECT_HPP_

#include <mutex>
#include <unordered_map>

#include <windows.h>
#include <winhttp.h>

#include "infrastructure/hook.hpp"
#include "infrastructure/http_redirect_rules.hpp"

namespace crabe::infrastructure {

class HttpRedirect {
    public:
        // Returns the singleton instance of HttpRedirect.
        static HttpRedirect& get();

        // Installs WinHTTP hooks when rules holds at least one redirection entry.
        // Returns false with nothing hooked if WinHTTP cannot be hooked.
        bool initialize(HttpRedirectRules rules);

        // Uninstalls all WinHTTP hooks and resets state.
        void uninitialize();

    private:
        HttpRedirect() = default;
        ~HttpRedirect() = default;
        HttpRedirect(const HttpRedirect&) = delete;
        HttpRedirect& operator=(const HttpRedirect&) = delete;

        using t_Connect = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
        using t_OpenRequest = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
        using t_CloseHandle = BOOL(WINAPI*)(HINTERNET);

        // Hook detour for WinHttpConnect.
        static HINTERNET WINAPI hkConnect(HINTERNET session, LPCWSTR serverName, INTERNET_PORT port, DWORD reserved);

        // Hook detour for WinHttpOpenRequest.
        static HINTERNET WINAPI hkOpenRequest(HINTERNET connection, LPCWSTR verb, LPCWSTR objectName,
                                              LPCWSTR version, LPCWSTR referrer, LPCWSTR* acceptTypes, DWORD flags);

        // Hook detour for WinHttpCloseHandle.
        static BOOL WINAPI hkCloseHandle(HINTERNET handle);

        Hook _connect;
        Hook _openRequest;
        Hook _closeHandle;

        std::mutex _mutex;
        HttpRedirectRules _rules;
        std::unordered_map<HINTERNET, HttpRedirectTarget> _connections;
};

} // namespace crabe::infrastructure

#endif /* !HTTP_REDIRECT_HPP_ */
