/*
** CrabeLoader
** File description:
** WinUSB access to a Disney Infinity base for the Xbox 360 and Xbox One transports.
** Detects presence and the bound driver, and finds the interface through the registered GUID.
** Reads and writes the interrupt or bulk pipes with per-pipe timeouts.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/usb_device.hpp"

#include <combaseapi.h>
#include <setupapi.h>
#include <winusb.h>

#include <cwctype>
#include <format>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")
#pragma comment(lib, "ole32.lib")

namespace crabe::infrastructure::portal {

namespace {

constexpr GUID kGuidDevInterfaceUsbDevice = {0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}};

// Returns text in lower case, for case-insensitive device path matching.
std::wstring Lower(std::wstring text) {
    for (auto& c : text) c = static_cast<wchar_t>(std::towlower(c));
    return text;
}

// Returns the lower-case "vid_xxxx&pid_xxxx" token found in device interface paths.
std::wstring VidPidToken(std::uint16_t vid, std::uint16_t pid) {
    return Lower(std::format(L"vid_{:04x}&pid_{:04x}", vid, pid));
}

// Interface paths of every present device exposing the given interface class GUID.
std::vector<std::wstring> EnumeratePaths(const GUID& guid) {
    std::vector<std::wstring> paths;
    HDEVINFO info = SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) return paths;

    SP_DEVICE_INTERFACE_DATA data{};
    data.cbSize = sizeof(data);
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(info, nullptr, &guid, index, &data); ++index) {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetailW(info, &data, nullptr, 0, &needed, nullptr);
        if (needed < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;

        std::vector<std::uint8_t> storage(needed);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (SetupDiGetDeviceInterfaceDetailW(info, &data, detail, needed, nullptr, nullptr)) {
            paths.emplace_back(detail->DevicePath);
        }
    }
    SetupDiDestroyDeviceInfoList(info);
    return paths;
}

bool ParseGuid(const std::wstring& text, GUID& guid) {
    return CLSIDFromString(text.c_str(), &guid) == S_OK;
}

// DeviceInterfaceGUIDs written by the WinUSB INF for every instance of this VID/PID.
std::vector<GUID> WinUsbInterfaceGuids(std::uint16_t vid, std::uint16_t pid) {
    std::vector<GUID> guids;
    const std::wstring base = std::format(L"SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_{:04X}&PID_{:04X}", vid, pid);
    HKEY root = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, base.c_str(), 0, KEY_READ, &root) != ERROR_SUCCESS) return guids;

    wchar_t instance[256];
    for (DWORD i = 0;; ++i) {
        DWORD length = static_cast<DWORD>(std::size(instance));
        if (RegEnumKeyExW(root, i, instance, &length, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

        HKEY params = nullptr;
        const std::wstring sub = std::wstring(instance) + L"\\Device Parameters";
        if (RegOpenKeyExW(root, sub.c_str(), 0, KEY_READ, &params) != ERROR_SUCCESS) continue;

        DWORD type = 0;
        DWORD size = 0;
        if (RegQueryValueExW(params, L"DeviceInterfaceGUIDs", nullptr, &type, nullptr, &size) == ERROR_SUCCESS && size > 0 &&
            (type == REG_MULTI_SZ || type == REG_SZ)) {
            std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1, L'\0');
            if (RegQueryValueExW(params, L"DeviceInterfaceGUIDs", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buffer.data()), &size) == ERROR_SUCCESS) {
                for (const wchar_t* p = buffer.data(); *p; p += wcslen(p) + 1) {
                    GUID guid{};
                    if (ParseGuid(p, guid)) guids.push_back(guid);
                }
            }
        }
        RegCloseKey(params);
    }
    RegCloseKey(root);
    return guids;
}

} // namespace

UsbDevice::~UsbDevice() {
    Close();
}

bool UsbDevice::IsPresent(std::uint16_t vid, std::uint16_t pid) {
    const std::wstring token = VidPidToken(vid, pid);
    for (const auto& path : EnumeratePaths(kGuidDevInterfaceUsbDevice)) {
        if (Lower(path).find(token) != std::wstring::npos) return true;
    }
    return false;
}

bool UsbDevice::HasWinUsbDriver(std::uint16_t vid, std::uint16_t pid) {
    const std::wstring token = VidPidToken(vid, pid);
    for (const GUID& guid : WinUsbInterfaceGuids(vid, pid)) {
        for (const auto& path : EnumeratePaths(guid)) {
            if (Lower(path).find(token) != std::wstring::npos) return true;
        }
    }
    return false;
}

bool UsbDevice::Open(std::uint16_t vid, std::uint16_t pid) {
    Close();
    const std::wstring token = VidPidToken(vid, pid);

    for (const GUID& guid : WinUsbInterfaceGuids(vid, pid)) {
        for (const auto& path : EnumeratePaths(guid)) {
            if (Lower(path).find(token) == std::wstring::npos) continue;

            HANDLE file = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
            if (file == INVALID_HANDLE_VALUE) continue;

            WINUSB_INTERFACE_HANDLE handle = nullptr;
            if (!WinUsb_Initialize(file, &handle)) {
                CloseHandle(file);
                continue;
            }

            USB_INTERFACE_DESCRIPTOR descriptor{};
            if (!WinUsb_QueryInterfaceSettings(handle, 0, &descriptor)) {
                WinUsb_Free(handle);
                CloseHandle(file);
                continue;
            }

            UCHAR in = 0;
            UCHAR out = 0;
            std::size_t inMax = 0;
            std::size_t outMax = 0;
            for (UCHAR i = 0; i < descriptor.bNumEndpoints; ++i) {
                WINUSB_PIPE_INFORMATION pipe{};
                if (!WinUsb_QueryPipe(handle, 0, i, &pipe)) continue;
                if (pipe.PipeType != UsbdPipeTypeInterrupt && pipe.PipeType != UsbdPipeTypeBulk) continue;
                if (USB_ENDPOINT_DIRECTION_IN(pipe.PipeId) && !in) {
                    in = pipe.PipeId;
                    inMax = pipe.MaximumPacketSize;
                } else if (USB_ENDPOINT_DIRECTION_OUT(pipe.PipeId) && !out) {
                    out = pipe.PipeId;
                    outMax = pipe.MaximumPacketSize;
                }
            }
            if (!in || !out) {
                WinUsb_Free(handle);
                CloseHandle(file);
                continue;
            }

            m_file = file;
            m_winusb = handle;
            m_inPipe = in;
            m_outPipe = out;
            m_inMax = inMax;
            m_outMax = outMax;
            m_inTimeout = m_outTimeout = 0xFFFFFFFFU;
            return true;
        }
    }
    return false;
}

void UsbDevice::Close() {
    if (m_winusb) {
        WinUsb_Free(m_winusb);
        m_winusb = nullptr;
    }
    if (m_file != INVALID_HANDLE_VALUE) {
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
    }
}

void UsbDevice::SetTimeout(UCHAR pipe, unsigned ms) {
    unsigned& cached = (pipe == m_inPipe) ? m_inTimeout : m_outTimeout;
    if (cached == ms) return;
    ULONG value = ms;
    WinUsb_SetPipePolicy(m_winusb, pipe, PIPE_TRANSFER_TIMEOUT, sizeof(value), &value);
    cached = ms;
}

UsbDevice::IoResult UsbDevice::Classify(DWORD error) const {
    switch (error) {
        case ERROR_SEM_TIMEOUT:
        case ERROR_TIMEOUT:
        case ERROR_OPERATION_ABORTED:
        case WAIT_TIMEOUT:
            return IoResult::Timeout;
        default:
            return IoResult::Lost;
    }
}

UsbDevice::IoResult UsbDevice::Read(std::span<std::uint8_t> buffer, unsigned timeoutMs, std::size_t& received) {
    received = 0;
    if (!m_winusb) return IoResult::Lost;
    SetTimeout(m_inPipe, timeoutMs);

    ULONG count = 0;
    if (WinUsb_ReadPipe(m_winusb, m_inPipe, buffer.data(), static_cast<ULONG>(buffer.size()), &count, nullptr)) {
        received = count;
        return IoResult::Ok;
    }
    return Classify(GetLastError());
}

UsbDevice::IoResult UsbDevice::Write(std::span<const std::uint8_t> data, unsigned timeoutMs) {
    if (!m_winusb) return IoResult::Lost;
    SetTimeout(m_outPipe, timeoutMs);

    ULONG count = 0;
    if (WinUsb_WritePipe(m_winusb, m_outPipe, const_cast<PUCHAR>(data.data()), static_cast<ULONG>(data.size()), &count,
                         nullptr)) {
        return IoResult::Ok;
    }
    return Classify(GetLastError());
}

} // namespace crabe::infrastructure::portal
