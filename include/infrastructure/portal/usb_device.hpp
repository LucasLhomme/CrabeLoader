#pragma once

#include <windows.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace crabe::infrastructure::portal {

// A USB device opened through WinUSB. The device must have the WinUSB driver bound to it (for
// example with Zadig); the interface GUID the INF registered is read from the registry.
class UsbDevice final {
public:
    enum class IoResult { Ok, Timeout, Lost };

    UsbDevice() = default;
    ~UsbDevice();
    UsbDevice(const UsbDevice&) = delete;
    UsbDevice& operator=(const UsbDevice&) = delete;

    // True when a USB device with this VID/PID is present, whatever driver it uses.
    [[nodiscard]] static bool IsPresent(std::uint16_t vid, std::uint16_t pid);
    // True when a WinUSB interface is registered for this VID/PID.
    [[nodiscard]] static bool HasWinUsbDriver(std::uint16_t vid, std::uint16_t pid);

    // Opens the first interface-0 WinUSB device matching VID/PID and its interrupt/bulk pipes.
    [[nodiscard]] bool Open(std::uint16_t vid, std::uint16_t pid);
    void Close();
    [[nodiscard]] bool IsOpen() const { return m_winusb != nullptr; }

    [[nodiscard]] IoResult Read(std::span<std::uint8_t> buffer, unsigned timeoutMs, std::size_t& received);
    [[nodiscard]] IoResult Write(std::span<const std::uint8_t> data, unsigned timeoutMs);

    [[nodiscard]] std::size_t InMaxPacket() const { return m_inMax; }
    [[nodiscard]] std::size_t OutMaxPacket() const { return m_outMax; }

private:
    IoResult Classify(DWORD error) const;
    void SetTimeout(UCHAR pipe, unsigned ms);

    HANDLE m_file{INVALID_HANDLE_VALUE};
    void* m_winusb{nullptr};
    UCHAR m_inPipe{0};
    UCHAR m_outPipe{0};
    std::size_t m_inMax{0};
    std::size_t m_outMax{0};
    unsigned m_inTimeout{0xFFFFFFFFU};
    unsigned m_outTimeout{0xFFFFFFFFU};
};

} // namespace crabe::infrastructure::portal
