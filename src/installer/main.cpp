/*
** CrabeLoader
** File description:
** The CrabeInstaller.exe entry point: find the game, ask once, install, say what happened.
** Windows dialogs only (IFileOpenDialog, message boxes), in French or English by the UI language.
** Decides nothing about the folder; installer/game_installer.cpp does, and refuses what is unsafe.
**
** Authors: @LucasLhomme
*/

#include <cstddef>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "installer/game_installer.hpp"
#include "installer/steam_locator.hpp"
#include "shared/version.hpp"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "uuid.lib")

namespace {

    using namespace crabe::installer;

    constexpr int kExitInstalled = 0;
    constexpr int kExitFailed = 1;
    constexpr int kExitCancelled = 2;
    constexpr WORD kPayloadResourceId = 101;

    struct Texts {
        std::wstring_view title;
        std::wstring_view detected;
        std::wstring_view pickTitle;
        std::wstring_view notGameFolder;
        std::wstring_view installed;
        std::wstring_view updated;
        std::wstring_view proxyUnrecognized;
        std::wstring_view proxyMissing;
        std::wstring_view cannotWrite;
        std::wstring_view payloadInvalid;
        std::wstring_view ioFailure;
    };

    constexpr Texts kFrench{
        L"Installation de CrabeLoader",
        L"Disney Infinity 3.0 a été détecté dans :\n\n{}\n\nInstaller CrabeLoader ici ?\n\n"
        L"Oui : installer ici\nNon : choisir un autre dossier\nAnnuler : quitter",
        L"Choisissez le dossier de Disney Infinity 3.0 (celui qui contient DisneyInfinity3.exe)",
        L"DisneyInfinity3.exe est introuvable dans :\n\n{}\n\nChoisissez le dossier qui contient le jeu.",
        L"CrabeLoader {} a été installé dans :\n\n{}\n\n"
        L"Le dossier « mods » est prêt : lisez mods\\README.txt pour y ajouter vos mods.\n\n"
        L"Lancez le jeu pour commencer.",
        L"CrabeLoader a été mis à jour vers la version {} dans :\n\n{}\n\n"
        L"Vos mods et votre configuration n'ont pas été touchés.",
        L"Le fichier bink2w32.dll de ce dossier n'est pas celui du jeu, et CrabeLoader n'y est pas déjà installé.\n\n"
        L"Aucun fichier n'a été modifié.\n\n"
        L"Dans Steam : clic droit sur le jeu > Propriétés > Fichiers installés > "
        L"Vérifier l'intégrité des fichiers, puis relancez cet installeur.",
        L"Le fichier bink2w32.dll est introuvable dans ce dossier.\n\n"
        L"Aucun fichier n'a été modifié.\n\n"
        L"Dans Steam : clic droit sur le jeu > Propriétés > Fichiers installés > "
        L"Vérifier l'intégrité des fichiers, puis relancez cet installeur.",
        L"Impossible d'écrire dans le dossier du jeu.\n\n"
        L"Fermez Disney Infinity 3.0 s'il est lancé. Si le jeu est dans un dossier protégé, relancez l'installeur "
        L"en administrateur (clic droit > Exécuter en tant qu'administrateur).\n\n"
        L"Aucun fichier du jeu n'a été perdu.\n\nDétail : {}",
        L"Cet installeur est incomplet : la DLL de CrabeLoader n'est pas incluse.\n\n"
        L"Téléchargez-le de nouveau depuis la page des versions.\n\nDétail : {}",
        L"L'installation a échoué.\n\nLes fichiers du jeu ont été remis dans leur état d'origine.\n\nDétail : {}",
    };

    constexpr Texts kEnglish{
        L"CrabeLoader installer",
        L"Disney Infinity 3.0 was found in:\n\n{}\n\nInstall CrabeLoader here?\n\n"
        L"Yes: install here\nNo: choose another folder\nCancel: quit",
        L"Choose the Disney Infinity 3.0 folder (the one containing DisneyInfinity3.exe)",
        L"DisneyInfinity3.exe was not found in:\n\n{}\n\nChoose the folder that contains the game.",
        L"CrabeLoader {} was installed in:\n\n{}\n\n"
        L"The \"mods\" folder is ready: read mods\\README.txt to add your mods.\n\n"
        L"Start the game to begin.",
        L"CrabeLoader was updated to version {} in:\n\n{}\n\n"
        L"Your mods and your configuration were not touched.",
        L"The bink2w32.dll in this folder is not the game's own, and CrabeLoader is not already installed.\n\n"
        L"No file was changed.\n\n"
        L"In Steam: right-click the game > Properties > Installed Files > Verify integrity of game files, "
        L"then run this installer again.",
        L"bink2w32.dll was not found in this folder.\n\n"
        L"No file was changed.\n\n"
        L"In Steam: right-click the game > Properties > Installed Files > Verify integrity of game files, "
        L"then run this installer again.",
        L"Cannot write to the game folder.\n\n"
        L"Close Disney Infinity 3.0 if it is running. If the game is in a protected folder, run the installer "
        L"as administrator (right-click > Run as administrator).\n\n"
        L"No game file was lost.\n\nDetails: {}",
        L"This installer is incomplete: the CrabeLoader DLL is not included.\n\n"
        L"Download it again from the releases page.\n\nDetails: {}",
        L"The installation failed.\n\nThe game files were put back as they were.\n\nDetails: {}",
    };

    struct Options {
        std::optional<std::filesystem::path> gameDir;
        std::optional<std::filesystem::path> payloadFile;
        bool quiet{false};
        bool help{false};
        bool valid{true};
    };

    struct LocalFreeCloser {
        void operator()(void* memory) const noexcept { LocalFree(memory); }
    };

    struct CoTaskMemCloser {
        void operator()(void* memory) const noexcept { CoTaskMemFree(memory); }
    };

    template <typename T>
    struct ComReleaser {
        void operator()(T* object) const noexcept { object->Release(); }
    };

    template <typename T>
    using ComPtr = std::unique_ptr<T, ComReleaser<T>>;

    class ComApartment final {
    public:
        ComApartment() noexcept
            : _initialized(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        {
        }

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

    template <typename... Args>
    [[nodiscard]] std::wstring formatText(std::wstring_view pattern, const Args&... args)
    {
        return std::vformat(pattern, std::make_wformat_args(args...));
    }

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

    [[nodiscard]] const Texts& chooseTexts() noexcept
    {
        return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_FRENCH ? kFrench : kEnglish;
    }

    [[nodiscard]] Options parseOptions()
    {
        Options options;
        int count = 0;
        std::unique_ptr<LPWSTR, LocalFreeCloser> argv(CommandLineToArgvW(GetCommandLineW(), &count));
        if (!argv)
            return options;

        for (int i = 1; i < count; ++i) {
            const std::wstring_view arg = argv.get()[i];
            if (arg == L"--quiet") {
                options.quiet = true;
            } else if (arg == L"--help" || arg == L"/?") {
                options.help = true;
            } else if (arg == L"--game-dir" && i + 1 < count) {
                options.gameDir = std::filesystem::path(argv.get()[++i]);
            } else if (arg == L"--payload" && i + 1 < count) {
                options.payloadFile = std::filesystem::path(argv.get()[++i]);
            } else {
                options.valid = false;
            }
        }
        return options;
    }

    int show(const Options& options, const Texts& texts, std::wstring_view body, UINT flags, int quietAnswer)
    {
        if (options.quiet)
            return quietAnswer;
        const std::wstring text(body);
        const std::wstring title(texts.title);
        return MessageBoxW(nullptr, text.c_str(), title.c_str(), flags | MB_SETFOREGROUND);
    }

    [[nodiscard]] std::optional<std::vector<std::byte>> readFileBytes(const std::filesystem::path& file)
    {
        std::ifstream stream(file, std::ios::binary);
        if (!stream.is_open())
            return std::nullopt;
        const std::vector<char> raw((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        std::vector<std::byte> bytes(raw.size());
        for (std::size_t i = 0; i < raw.size(); ++i)
            bytes[i] = static_cast<std::byte>(raw[i]);
        return bytes;
    }

    [[nodiscard]] std::optional<std::vector<std::byte>> readEmbeddedPayload()
    {
        HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(kPayloadResourceId), RT_RCDATA);
        if (!resource)
            return std::nullopt;
        HGLOBAL loaded = LoadResource(nullptr, resource);
        const DWORD size = SizeofResource(nullptr, resource);
        const void* data = loaded ? LockResource(loaded) : nullptr;
        if (!data || size == 0)
            return std::nullopt;

        const auto* begin = static_cast<const std::byte*>(data);
        return std::vector<std::byte>(begin, begin + size);
    }

    [[nodiscard]] std::filesystem::path executableDirectory()
    {
        std::wstring path(MAX_PATH, L'\0');
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        path.resize(length);
        return std::filesystem::path(path).parent_path();
    }

    [[nodiscard]] std::optional<std::vector<std::byte>> loadPayload(const Options& options)
    {
        if (options.payloadFile)
            return readFileBytes(*options.payloadFile);
        if (auto embedded = readEmbeddedPayload())
            return embedded;
        return readFileBytes(executableDirectory() / std::string(kProxyName));
    }

    [[nodiscard]] std::optional<std::filesystem::path> pickFolder(const Texts& texts,
                                                                  const std::optional<std::filesystem::path>& startAt)
    {
        IFileOpenDialog* rawDialog = nullptr;
        if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&rawDialog))))
            return std::nullopt;
        const ComPtr<IFileOpenDialog> dialog(rawDialog);

        DWORD flags = 0;
        dialog->GetOptions(&flags);
        dialog->SetOptions(flags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);

        const std::wstring title(texts.pickTitle);
        dialog->SetTitle(title.c_str());

        if (startAt) {
            IShellItem* rawStart = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(startAt->c_str(), nullptr, IID_PPV_ARGS(&rawStart)))) {
                const ComPtr<IShellItem> start(rawStart);
                dialog->SetFolder(start.get());
            }
        }

        if (FAILED(dialog->Show(nullptr)))
            return std::nullopt;

        IShellItem* rawResult = nullptr;
        if (FAILED(dialog->GetResult(&rawResult)))
            return std::nullopt;
        const ComPtr<IShellItem> result(rawResult);

        PWSTR rawName = nullptr;
        if (FAILED(result->GetDisplayName(SIGDN_FILESYSPATH, &rawName)))
            return std::nullopt;
        const std::unique_ptr<wchar_t, CoTaskMemCloser> name(rawName);
        return std::filesystem::path(name.get());
    }

    [[nodiscard]] std::wstring_view failureText(const Texts& texts, InstallErrorCode code) noexcept
    {
        switch (code) {
        case InstallErrorCode::ProxyUnrecognized:
            return texts.proxyUnrecognized;
        case InstallErrorCode::ProxyMissing:
            return texts.proxyMissing;
        case InstallErrorCode::FileInUse:
        case InstallErrorCode::AccessDenied:
            return texts.cannotWrite;
        case InstallErrorCode::PayloadInvalid:
            return texts.payloadInvalid;
        case InstallErrorCode::NotAGameFolder:
        case InstallErrorCode::IoFailure:
            return texts.ioFailure;
        }
        return texts.ioFailure;
    }

    [[nodiscard]] std::optional<std::filesystem::path> chooseFolder(const Options& options, const Texts& texts)
    {
        if (options.gameDir)
            return options.gameDir;

        const std::optional<std::filesystem::path> detected = locateSteamGame();
        if (!detected)
            return pickFolder(texts, std::nullopt);

        const int answer = show(options, texts, formatText(texts.detected, detected->wstring()),
                                MB_YESNOCANCEL | MB_ICONQUESTION, IDYES);
        if (answer == IDYES)
            return detected;
        if (answer == IDNO)
            return pickFolder(texts, detected);
        return std::nullopt;
    }

    int run(const Options& options)
    {
        const Texts& texts = chooseTexts();

        const auto payload = loadPayload(options);
        if (!payload) {
            show(options, texts, formatText(texts.payloadInvalid, std::wstring(L"bink2w32.dll")), MB_OK | MB_ICONERROR, IDOK);
            return kExitFailed;
        }

        std::optional<std::filesystem::path> folder = chooseFolder(options, texts);
        for (;;) {
            if (!folder)
                return kExitCancelled;

            const GameInstaller installer(*folder);
            if (!installer.inspect().hasGameExecutable) {
                show(options, texts, formatText(texts.notGameFolder, folder->wstring()), MB_OK | MB_ICONWARNING, IDOK);
                if (options.gameDir || options.quiet)
                    return kExitFailed;
                folder = pickFolder(texts, std::nullopt);
                continue;
            }

            const auto report = installer.install(*payload);
            if (!report) {
                show(options, texts, formatText(failureText(texts, report.error().code), widen(report.error().detail)),
                     MB_OK | MB_ICONERROR, IDOK);
                return kExitFailed;
            }

            const std::wstring version = widen(crabe::version::String);
            const std::wstring_view done = report->performed == Plan::Update ? texts.updated : texts.installed;
            show(options, texts, formatText(done, version, folder->wstring()), MB_OK | MB_ICONINFORMATION, IDOK);
            return kExitInstalled;
        }
    }

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    const Options options = parseOptions();
    if (options.help || !options.valid) {
        const std::wstring usage =
            L"CrabeInstaller [--game-dir <folder>] [--quiet] [--payload <bink2w32.dll>]\n\n"
            L"--game-dir  skip the folder dialog and use this folder\n"
            L"--quiet     show no window; the exit code tells the result (0 ok, 1 failed, 2 cancelled)\n"
            L"--payload   install this DLL instead of the bundled one";
        MessageBoxW(nullptr, usage.c_str(), L"CrabeInstaller", MB_OK | MB_ICONINFORMATION);
        return options.valid ? kExitInstalled : kExitFailed;
    }

    const ComApartment apartment;
    return run(options);
}
