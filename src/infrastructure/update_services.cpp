/*
** CrabeLoader
** File description:
** Implements the update check ports with WinHTTP, MessageBoxW and ShellExecuteExW.
** The message box defaults to "No" so a key pressed in game never opens the browser by accident.
** Reads no version and compares nothing; it only moves text and the player's answer.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/update_services.hpp"

#include <cstddef>
#include <format>
#include <memory>
#include <type_traits>

#include <windows.h>
#include <objbase.h>
#include <winhttp.h>
#include <shellapi.h>

#include "shared/version.hpp"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")

namespace crabe::infrastructure {

    namespace {

        constexpr std::size_t kMaxBodyBytes = 512 * 1024;
        constexpr int kResolveTimeoutMs = 5000;
        constexpr int kConnectTimeoutMs = 5000;
        constexpr int kSendTimeoutMs = 5000;
        constexpr int kReceiveTimeoutMs = 8000;
        constexpr std::string_view kHttpsPrefix = "https://";

        struct WinHttpHandleCloser {
            // Releases an active WinHTTP handle.
            void operator()(HINTERNET handle) const noexcept
            {
                if (handle)
                    WinHttpCloseHandle(handle);
            }
        };

        using WinHttpHandle = std::unique_ptr<std::remove_pointer_t<HINTERNET>, WinHttpHandleCloser>;

        class ComApartment final {
        public:
            // Initializes COM in apartment-threaded mode.
            ComApartment() noexcept
                : _initialized(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
            {
            }

            // Uninitializes COM if previously initialized.
            ~ComApartment()
            {
                if (_initialized)
                    CoUninitialize();
            }

            ComApartment(const ComApartment&) = delete;
            ComApartment& operator=(const ComApartment&) = delete;
            ComApartment(ComApartment&&) = delete;
            ComApartment& operator=(ComApartment&&) = delete;

        private:
            bool _initialized;
        };

        // Converts a UTF-8 string to a wide character string.
        [[nodiscard]] std::wstring widen(std::string_view text)
        {
            if (text.empty())
                return {};
            const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
            if (length <= 0)
                return {};
            std::wstring wide(static_cast<std::size_t>(length), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
            return wide;
        }

        // Formats an error string with the step description and GetLastError code.
        [[nodiscard]] std::string failure(std::string_view step)
        {
            return std::format("{} failed (error {})", step, GetLastError());
        }

        // Checks if the user's default UI language is French.
        [[nodiscard]] bool uiLanguageIsFrench() noexcept
        {
            return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_FRENCH;
        }

        // Opens a WinHTTP session with automatic proxy fallback.
        [[nodiscard]] WinHttpHandle openSession(const std::wstring& agent)
        {
            WinHttpHandle session(WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
            if (!session)
                session.reset(WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
            return session;
        }

        // Configures the WinHTTP session to allow only modern TLS versions.
        void restrictToModernTls(HINTERNET session) noexcept
        {
            DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
            protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
            WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
        }

        // Reads the complete response body from an active WinHTTP request.
        [[nodiscard]] std::expected<std::string, std::string> readBody(HINTERNET request)
        {
            std::string body;
            for (;;) {
                DWORD available = 0;
                if (!WinHttpQueryDataAvailable(request, &available))
                    return std::unexpected(failure("WinHttpQueryDataAvailable"));
                if (available == 0)
                    return body;
                if (body.size() + available > kMaxBodyBytes)
                    return std::unexpected(std::string("response larger than the allowed maximum"));

                const std::size_t offset = body.size();
                body.resize(offset + available);
                DWORD read = 0;
                if (!WinHttpReadData(request, body.data() + offset, available, &read))
                    return std::unexpected(failure("WinHttpReadData"));
                body.resize(offset + read);
            }
        }

    } // namespace

    // Fetches the latest release metadata from the GitHub API using WinHTTP.
    std::expected<std::string, std::string> WinHttpReleaseSource::fetchLatestRelease()
    {
        const std::wstring agent = widen(std::format("CrabeLoader/{}", crabe::version::String));
        WinHttpHandle session = openSession(agent);
        if (!session)
            return std::unexpected(failure("WinHttpOpen"));

        WinHttpSetTimeouts(session.get(), kResolveTimeoutMs, kConnectTimeoutMs, kSendTimeoutMs, kReceiveTimeoutMs);
        restrictToModernTls(session.get());

        const std::wstring host = widen(domain::kReleaseApiHost);
        WinHttpHandle connection(WinHttpConnect(session.get(), host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
        if (!connection)
            return std::unexpected(failure("WinHttpConnect"));

        const std::wstring path = widen(domain::kLatestReleasePath);
        WinHttpHandle request(WinHttpOpenRequest(connection.get(), L"GET", path.c_str(), nullptr,
                                                 WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 WINHTTP_FLAG_SECURE));
        if (!request)
            return std::unexpected(failure("WinHttpOpenRequest"));

        constexpr const wchar_t* kHeaders =
            L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
        if (!WinHttpSendRequest(request.get(), kHeaders, static_cast<DWORD>(-1L),
                                WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
            return std::unexpected(failure("WinHttpSendRequest"));
        if (!WinHttpReceiveResponse(request.get(), nullptr))
            return std::unexpected(failure("WinHttpReceiveResponse"));

        DWORD status = 0;
        DWORD statusSize = sizeof(status);
        if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                 WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX))
            return std::unexpected(failure("WinHttpQueryHeaders"));
        if (status != HTTP_STATUS_OK)
            return std::unexpected(std::format("HTTP status {}", status));

        return readBody(request.get());
    }

    // Displays a localized message box prompting the user to update.
    bool MessageBoxUpdatePrompt::askToUpdate(std::string_view currentVersion, std::string_view latestVersion)
    {
        const std::wstring current = widen(currentVersion);
        const std::wstring latest = widen(latestVersion);

        std::wstring title;
        std::wstring text;
        if (uiLanguageIsFrench()) {
            title = L"CrabeLoader - Mise à jour disponible";
            text = std::format(L"Une nouvelle version de CrabeLoader est disponible : {} (installée : {}).\n\n"
                               L"Voulez-vous mettre le modloader à jour ?\n"
                               L"Oui ouvre la page des versions dans votre navigateur.",
                               latest, current);
        } else {
            title = L"CrabeLoader - Update available";
            text = std::format(L"A new version of CrabeLoader is available: {} (installed: {}).\n\n"
                               L"Do you want to update the modloader?\n"
                               L"Yes opens the releases page in your browser.",
                               latest, current);
        }

        constexpr UINT kFlags = MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2 | MB_SETFOREGROUND | MB_TOPMOST;
        return MessageBoxW(nullptr, text.c_str(), title.c_str(), kFlags) == IDYES;
    }

    // Opens an HTTPS URL in the default browser using ShellExecuteExW.
    bool ShellUrlOpener::open(std::string_view url)
    {
        if (!url.starts_with(kHttpsPrefix))
            return false;

        const std::wstring wideUrl = widen(url);
        const ComApartment apartment;

        SHELLEXECUTEINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
        info.lpVerb = L"open";
        info.lpFile = wideUrl.c_str();
        info.nShow = SW_SHOWNORMAL;
        return ShellExecuteExW(&info) != FALSE;
    }

} // namespace crabe::infrastructure
