/*
** CrabeLoader
** File description:
** Implements IPortalDriver for Disney Infinity Bases:
** Universal HID Driver (PS3, PS4, Wii, Wii U, PC) + Device Detection for Xbox Bases.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_driver.hpp"
#include "infrastructure/portal/portal_crypto.hpp"

#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>

#include <array>
#include <chrono>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace crabe::infrastructure::portal {

namespace {

constexpr uint16_t kVidPdp = 0x0E6F;
constexpr uint16_t kPidStandardHid = 0x0129; // PS3 / PS4 / Wii / Wii U / PC
constexpr uint16_t kPidXboxOne = 0x0229;     // Xbox One (INF-8037059)

constexpr uint16_t kVidXbox360 = 0x24C6;
constexpr uint16_t kPidXbox360 = 0xFA00;     // Xbox 360 (INF-8032385)

// Disney Gateway Protocol command IDs
constexpr uint8_t kCmdWake = 0x80;
constexpr uint8_t kCmdSetLed = 0x90;
constexpr uint8_t kCmdReadBlock = 0xA2;
constexpr uint8_t kCmdTagArrival = 0xAB;
constexpr uint8_t kCmdGetUid = 0xB4;

uint8_t CalculateChecksum(const uint8_t* buffer, size_t length) {
    uint32_t sum = 0;
    for (size_t i = 0; i < length; ++i) {
        sum += buffer[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

// ------------------------------------------------------------- Standard HID Driver
class StandardHidPortalDriver final : public IPortalDriver {
public:
    explicit StandardHidPortalDriver(std::wstring devicePath)
        : m_devicePath(std::move(devicePath)) {}

    ~StandardHidPortalDriver() override {
        Close();
    }

    bool Open() override {
        if (m_hDevice != INVALID_HANDLE_VALUE) {
            return true;
        }

        m_hDevice = CreateFileW(
            m_devicePath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr
        );

        if (m_hDevice == INVALID_HANDLE_VALUE) {
            return false;
        }

        m_hReadEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        m_hWriteEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

        // Send Wake command
        SendWake();
        return true;
    }

    void Close() override {
        if (m_hDevice != INVALID_HANDLE_VALUE) {
            CancelIo(m_hDevice);
            CloseHandle(m_hDevice);
            m_hDevice = INVALID_HANDLE_VALUE;
        }
        if (m_hReadEvent != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hReadEvent);
            m_hReadEvent = INVALID_HANDLE_VALUE;
        }
        if (m_hWriteEvent != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hWriteEvent);
            m_hWriteEvent = INVALID_HANDLE_VALUE;
        }
    }

    [[nodiscard]] bool IsConnected() const override {
        return m_hDevice != INVALID_HANDLE_VALUE;
    }

    [[nodiscard]] PortalType GetType() const override {
        return PortalType::StandardHid;
    }

    [[nodiscard]] const char* GetTypeDescription() const override {
        return "Portail Universel USB HID (PS3/PS4/Wii/Wii U/PC)";
    }

    bool SetLed(uint8_t slot, uint8_t r, uint8_t g, uint8_t b) override {
        if (!IsConnected()) return false;

        // Packet format: [ReportId=0, 0xFF, len=6, 0x90, seq, slot, R, G, B, checksum, padding...]
        std::array<uint8_t, 33> pkt{};
        pkt[0] = 0x00; // HID Report ID
        pkt[1] = 0xFF;
        pkt[2] = 0x06;
        pkt[3] = kCmdSetLed;
        pkt[4] = ++m_sequence;
        pkt[5] = slot;
        pkt[6] = r;
        pkt[7] = g;
        pkt[8] = b;
        pkt[9] = CalculateChecksum(&pkt[1], 8);

        return WritePacket(pkt.data(), pkt.size());
    }

    void Poll(std::vector<PortalTagEvent>& outEvents) override {
        if (!IsConnected()) return;

        std::array<uint8_t, 64> rxBuf{};
        DWORD bytesRead = 0;
        OVERLAPPED ov{};
        ov.hEvent = m_hReadEvent;
        ResetEvent(m_hReadEvent);

        if (!ReadFile(m_hDevice, rxBuf.data(), static_cast<DWORD>(rxBuf.size()), &bytesRead, &ov)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                if (WaitForSingleObject(m_hReadEvent, 10) == WAIT_OBJECT_0) {
                    GetOverlappedResult(m_hDevice, &ov, &bytesRead, FALSE);
                } else {
                    CancelIo(m_hDevice);
                    return;
                }
            } else {
                return;
            }
        }

        if (bytesRead < 4) return;

        // Skip leading 0x00 HID report ID if present
        const uint8_t* p = rxBuf.data();
        if (p[0] == 0x00) {
            p++;
            bytesRead--;
        }

        if (p[0] != 0xFF) return;

        uint8_t cmd = p[2];
        if (cmd == kCmdTagArrival) {
            // Tag Arrival: antenna index and LTI
            uint8_t antenna = p[4]; // 1=P1, 2=P2, 3=Playset
            uint8_t eventType = p[5];
            uint8_t lti = p[6];

            if (eventType == 0) {
                // Placed: queue read for Block 1 (ID Block)
                RequestReadBlock(lti, 0, 1);
                m_activeLtiToSlot[lti] = antenna;
            } else {
                // Removed
                PortalTagEvent ev{};
                ev.type = PortalTagEvent::Type::Removed;
                ev.slot = antenna;
                ev.sku = 0;
                outEvents.push_back(ev);
                m_activeLtiToSlot.erase(lti);
            }
        } else if (cmd == kCmdReadBlock) {
            // Block 1 read response: 16 bytes encrypted
            uint8_t lti = p[4];
            auto it = m_activeLtiToSlot.find(lti);
            if (it != m_activeLtiToSlot.end() && bytesRead >= 21) {
                std::array<uint8_t, 16> encBlock{};
                std::memcpy(encBlock.data(), &p[5], 16);

                // For test/dummy UID derivation if 7-byte UID pending
                std::array<uint8_t, 7> uid = { 0x04, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc };
                auto sku = m_crypto.DecryptCharacterSku(uid, encBlock);
                if (sku.has_value()) {
                    PortalTagEvent ev{};
                    ev.type = PortalTagEvent::Type::Placed;
                    ev.slot = it->second;
                    ev.sku = *sku;
                    outEvents.push_back(ev);
                }
            }
        }
    }

private:
    bool SendWake() {
        std::array<uint8_t, 33> pkt{};
        pkt[0] = 0x00; // Report ID
        pkt[1] = 0xFF;
        pkt[2] = 0x11;
        pkt[3] = kCmdWake;
        pkt[4] = ++m_sequence;
        const char copyright[] = "(c) Disney 2013";
        std::memcpy(&pkt[5], copyright, 15);
        pkt[20] = CalculateChecksum(&pkt[1], 19);
        return WritePacket(pkt.data(), pkt.size());
    }

    bool RequestReadBlock(uint8_t lti, uint8_t sector, uint8_t block) {
        std::array<uint8_t, 33> pkt{};
        pkt[0] = 0x00;
        pkt[1] = 0xFF;
        pkt[2] = 0x05;
        pkt[3] = kCmdReadBlock;
        pkt[4] = ++m_sequence;
        pkt[5] = lti;
        pkt[6] = sector;
        pkt[7] = block;
        pkt[8] = CalculateChecksum(&pkt[1], 7);
        return WritePacket(pkt.data(), pkt.size());
    }

    bool WritePacket(const uint8_t* data, size_t size) {
        if (m_hDevice == INVALID_HANDLE_VALUE) return false;
        DWORD written = 0;
        OVERLAPPED ov{};
        ov.hEvent = m_hWriteEvent;
        ResetEvent(m_hWriteEvent);

        if (!WriteFile(m_hDevice, data, static_cast<DWORD>(size), &written, &ov)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                if (WaitForSingleObject(m_hWriteEvent, 200) == WAIT_OBJECT_0) {
                    GetOverlappedResult(m_hDevice, &ov, &written, FALSE);
                    return written > 0;
                }
                CancelIo(m_hDevice);
                return false;
            }
            return false;
        }
        return written > 0;
    }

    std::wstring m_devicePath;
    HANDLE m_hDevice{INVALID_HANDLE_VALUE};
    HANDLE m_hReadEvent{INVALID_HANDLE_VALUE};
    HANDLE m_hWriteEvent{INVALID_HANDLE_VALUE};
    uint8_t m_sequence{0};
    PortalCrypto m_crypto;
    std::unordered_map<uint8_t, uint8_t> m_activeLtiToSlot;
};

// ------------------------------------------------------------- Locked Xbox One Driver
class XboxOneLockedPortalDriver final : public IPortalDriver {
public:
    bool Open() override { return true; }
    void Close() override {}
    [[nodiscard]] bool IsConnected() const override { return true; }
    [[nodiscard]] PortalType GetType() const override { return PortalType::XboxOneGipLocked; }
    [[nodiscard]] const char* GetTypeDescription() const override {
        return "Portail Xbox One (INF-8037059) [Verrouillé matériellement]";
    }
    bool SetLed(uint8_t, uint8_t, uint8_t, uint8_t) override { return false; }
    void Poll(std::vector<PortalTagEvent>&) override {}
};

} // namespace

std::unique_ptr<IPortalDriver> CreatePortalDriver() {
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO hDevInfo = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo != INVALID_HANDLE_VALUE) {
        SP_DEVICE_INTERFACE_DATA ifData{};
        ifData.cbSize = sizeof(ifData);

        for (DWORD idx = 0; SetupDiEnumDeviceInterfaces(hDevInfo, nullptr, &hidGuid, idx, &ifData); ++idx) {
            DWORD reqLen = 0;
            SetupDiGetDeviceInterfaceDetailW(hDevInfo, &ifData, nullptr, 0, &reqLen, nullptr);
            if (reqLen > 0) {
                std::vector<uint8_t> detailBuf(reqLen);
                auto* detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailBuf.data());
                detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

                if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &ifData, detail, reqLen, nullptr, nullptr)) {
                    std::wstring path = detail->DevicePath;
                    // Lowercase check
                    std::wstring lowerPath = path;
                    for (auto& c : lowerPath) c = towlower(c);

                    if (lowerPath.find(L"vid_0e6f") != std::wstring::npos &&
                        lowerPath.find(L"pid_0129") != std::wstring::npos) {
                        SetupDiDestroyDeviceInfoList(hDevInfo);
                        auto driver = std::make_unique<StandardHidPortalDriver>(std::move(path));
                        if (driver->Open()) {
                            return driver;
                        }
                    }
                }
            }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    // Check if Xbox One device is connected via USB device path
    // VID: 0x0E6F, PID: 0x0229
    HANDLE hCheck = CreateFileW(
        L"\\\\?\\USB#VID_0E6F&PID_0229#0000359F89AE3D30#{a5dcbf10-6530-11d2-901f-00c04fb951ed}",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr
    );
    if (hCheck != INVALID_HANDLE_VALUE) {
        CloseHandle(hCheck);
        return std::make_unique<XboxOneLockedPortalDriver>();
    }

    return nullptr;
}

} // namespace crabe::infrastructure::portal
