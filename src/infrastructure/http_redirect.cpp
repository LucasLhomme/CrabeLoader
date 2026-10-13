/*
** CrabeLoader
** File description:
** Detours WinHttpConnect, WinHttpOpenRequest and WinHttpCloseHandle to apply the [http.redirect] rules.
** A connection keeps its target from Connect to CloseHandle so OpenRequest knows whether to drop TLS.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/http_redirect.hpp"

#include <optional>
#include <utility>

#include "shared/logger.hpp"

namespace crabe::infrastructure {

HttpRedirect& HttpRedirect::get()
{
    static HttpRedirect instance;
    return instance;
}

bool HttpRedirect::initialize(HttpRedirectRules rules)
{
    auto& logger = crabe::shared::Logger::getInstance();
    if (rules.empty())
        return true;

    HMODULE module = GetModuleHandleW(L"winhttp.dll");
    if (!module)
        module = LoadLibraryW(L"winhttp.dll");
    void* connect = module ? reinterpret_cast<void*>(GetProcAddress(module, "WinHttpConnect")) : nullptr;
    void* openRequest = module ? reinterpret_cast<void*>(GetProcAddress(module, "WinHttpOpenRequest")) : nullptr;
    void* closeHandle = module ? reinterpret_cast<void*>(GetProcAddress(module, "WinHttpCloseHandle")) : nullptr;
    if (!connect || !openRequest || !closeHandle) {
        logger.error("HttpRedirect: winhttp.dll or one of its exports is missing; no connection is redirected.");
        return false;
    }

    {
        std::lock_guard lock(_mutex);
        _rules = std::move(rules);
    }
    const bool ok = _closeHandle.install(closeHandle, reinterpret_cast<void*>(&HttpRedirect::hkCloseHandle),
                                         "HttpRedirect::WinHttpCloseHandle") &&
                    _openRequest.install(openRequest, reinterpret_cast<void*>(&HttpRedirect::hkOpenRequest),
                                         "HttpRedirect::WinHttpOpenRequest") &&
                    _connect.install(connect, reinterpret_cast<void*>(&HttpRedirect::hkConnect),
                                     "HttpRedirect::WinHttpConnect");
    if (!ok) {
        logger.error("HttpRedirect: could not hook WinHTTP; no connection is redirected.");
        uninitialize();
        return false;
    }
    logger.info("HttpRedirect: active with {} rule(s).", _rules.size());
    return true;
}

void HttpRedirect::uninitialize()
{
    _connect.remove();
    _openRequest.remove();
    _closeHandle.remove();
    std::lock_guard lock(_mutex);
    _connections.clear();
    _rules = HttpRedirectRules{};
}

HINTERNET WINAPI HttpRedirect::hkConnect(HINTERNET session, LPCWSTR serverName, INTERNET_PORT port, DWORD reserved)
{
    HttpRedirect& self = get();
    const auto original = reinterpret_cast<t_Connect>(self._connect.getOriginal());

    std::optional<HttpRedirectTarget> target;
    {
        std::lock_guard lock(self._mutex);
        if (serverName)
            target = self._rules.find(serverName);
    }
    if (!target)
        return original(session, serverName, port, reserved);

    HINTERNET connection = original(session, target->host.c_str(), target->port, reserved);
    crabe::shared::Logger::getInstance().info("HttpRedirect: {}:{} -> {}://{}:{}{}", narrowForLog(serverName), port,
                                              target->secure ? "https" : "http", narrowForLog(target->host),
                                              target->port, connection ? "" : " (WinHttpConnect failed)");
    if (connection) {
        std::lock_guard lock(self._mutex);
        self._connections.insert_or_assign(connection, std::move(*target));
    }
    return connection;
}

HINTERNET WINAPI HttpRedirect::hkOpenRequest(HINTERNET connection, LPCWSTR verb, LPCWSTR objectName,
                                             LPCWSTR version, LPCWSTR referrer, LPCWSTR* acceptTypes, DWORD flags)
{
    HttpRedirect& self = get();
    bool redirected = false;
    bool secure = false;
    {
        std::lock_guard lock(self._mutex);
        if (const auto it = self._connections.find(connection); it != self._connections.end()) {
            redirected = true;
            secure = it->second.secure;
        }
    }
    if (redirected) {
        flags = secure ? (flags | WINHTTP_FLAG_SECURE) : (flags & ~static_cast<DWORD>(WINHTTP_FLAG_SECURE));
        crabe::shared::Logger::getInstance().debug("HttpRedirect: {} {}", narrowForLog(verb ? verb : L"GET"),
                                                   narrowForLog(objectName ? objectName : L"/"));
    }
    const auto original = reinterpret_cast<t_OpenRequest>(self._openRequest.getOriginal());
    return original(connection, verb, objectName, version, referrer, acceptTypes, flags);
}

BOOL WINAPI HttpRedirect::hkCloseHandle(HINTERNET handle)
{
    HttpRedirect& self = get();
    {
        std::lock_guard lock(self._mutex);
        self._connections.erase(handle);
    }
    const auto original = reinterpret_cast<t_CloseHandle>(self._closeHandle.getOriginal());
    return original(handle);
}

} // namespace crabe::infrastructure
