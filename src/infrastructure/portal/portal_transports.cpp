/*
** CrabeLoader
** File description:
** Portal transports for the HID bases (PS3, PS4, Wii, Wii U, PC) and the Xbox 360 base over WinUSB.
** Also discovers every plugged-in base and builds the transport that fits it.
** The Xbox One transport lives in portal_transport_gip.cpp.
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

class HidTransport final : public IPortalTransport {
public:
    explicit HidTransport(std::wstring path) : m_path(std::move(path)) {}
    ~HidTransport() override { Close(); }

    bool Open() override {
        Close();
        m_device = CreateFileW(m_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (m_device == INVALID_HANDLE_VALUE) {
            m_diagnostic = "HID Disney Infinity base found but it cannot be opened (already in use by another program?).";
            return false;
        }

        PHIDP_PREPARSED_DATA preparsed = nullptr;
        HIDP_CAPS caps{};
        const bool haveCaps = HidD_GetPreparsedData(m_device, &preparsed) &&
                              HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS;
        if (preparsed) HidD_FreePreparsedData(preparsed);
        if (!haveCaps || caps.InputReportByteLength < kReportSize + 1 || caps.OutputReportByteLength < kReportSize + 1) {
            m_diagnostic = "This HID device does not use the 32-byte reports of a Disney Infinity base.";
            Close();
            return false;
        }
        m_inLen = caps.InputReportByteLength;
        m_outLen = caps.OutputReportByteLength;
        m_readBuffer.assign(m_inLen, 0);

        m_readEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        m_writeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!m_readEvent || !m_writeEvent) {
            Close();
            return false;
        }
        return true;
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

    // Sends the report behind a zero HID report id byte, giving up after kWriteTimeoutMs.
    bool Write(const Report& report) override {
        if (m_device == INVALID_HANDLE_VALUE) return false;
        std::vector<std::uint8_t> out(m_outLen, 0);
        std::copy(report.begin(), report.end(), out.begin() + 1);

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

    // Keeps one overlapped read pending across calls and strips the leading report id byte.
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
    std::string Description() const override { return "USB HID base (PS3 / PS4 / Wii / Wii U / PC)"; }
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

class Xbox360Transport final : public IPortalTransport {
public:
    bool Open() override {
        if (!m_usb.Open(kVidXbox360, kPidXbox360)) {
            m_diagnostic = "Xbox 360 base found but it cannot be opened through WinUSB.";
            return false;
        }
        return true;
    }
    void Close() override { m_usb.Close(); }

    // Sends a 32-byte packet made of the 0x0B 0x15 header and the first 30 bytes of the report.
    // No request the protocol builds is longer than 30 bytes.
    bool Write(const Report& report) override {
        std::array<std::uint8_t, kReportSize> packet{};
        packet[0] = 0x0B;
        packet[1] = 0x15;
        std::copy_n(report.begin(), kReportSize - 2, packet.begin() + 2);
        return m_usb.Write(packet, kWriteTimeoutMs) == UsbDevice::IoResult::Ok;
    }

    // Reads one packet and copies it into the report without its two-byte header.
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
    std::string Description() const override { return "Xbox 360 base (WinUSB)"; }
    std::string Diagnostic() const override { return m_diagnostic; }

private:
    UsbDevice m_usb;
    std::string m_diagnostic;
};

// Returns text in lower case, for case-insensitive device path matching.
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
            result.diagnostic = "Xbox 360 base found without the WinUSB driver: install it with Zadig (see docs/guides/portal.md).";
        }
    }

    if (UsbDevice::IsPresent(kVidPdp, kPidXboxOne)) {
        if (UsbDevice::HasWinUsbDriver(kVidPdp, kPidXboxOne)) {
            result.transports.push_back(MakeXboxOneTransport());
        } else {
            result.diagnostic = "Xbox One base found without the WinUSB driver: install it with Zadig (see docs/guides/portal.md).";
        }
    }
    return result;
}

} // namespace crabe::infrastructure::portal
