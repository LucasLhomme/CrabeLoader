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

// Hooks WinHttpConnect (server name and port swapped), WinHttpOpenRequest (TLS flag
// dropped for a plain-HTTP target) and WinHttpCloseHandle (forgets the connection).
// Request paths, headers and bodies are untouched.
class HttpRedirect {
    public:
        static HttpRedirect& get();

        // Installs the three hooks when `rules` holds at least one rule.
        // False, with nothing left hooked, when WinHTTP cannot be found or hooked.
        bool initialize(HttpRedirectRules rules);
        void uninitialize();

    private:
        HttpRedirect() = default;
        ~HttpRedirect() = default;
        HttpRedirect(const HttpRedirect&) = delete;
        HttpRedirect& operator=(const HttpRedirect&) = delete;

        using t_Connect = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
        using t_OpenRequest = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
        using t_CloseHandle = BOOL(WINAPI*)(HINTERNET);

        static HINTERNET WINAPI hkConnect(HINTERNET session, LPCWSTR serverName, INTERNET_PORT port, DWORD reserved);
        static HINTERNET WINAPI hkOpenRequest(HINTERNET connection, LPCWSTR verb, LPCWSTR objectName,
                                              LPCWSTR version, LPCWSTR referrer, LPCWSTR* acceptTypes, DWORD flags);
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
