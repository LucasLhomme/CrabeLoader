/*
** CrabeLoader
** File description:
** Declares UsbDevice, a USB device opened through WinUSB with timed pipe I/O.
** Detects whether a VID/PID is plugged in and whether WinUSB is its current driver.
** Used by the Xbox 360 and Xbox One portal transports.
**
** Authors: @LucasLhomme
*/

#pragma once

#include <windows.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace crabe::infrastructure::portal {

class UsbDevice final {
public:
    enum class IoResult { Ok, Timeout, Lost };

    UsbDevice() = default;
    ~UsbDevice();
    UsbDevice(const UsbDevice&) = delete;
    UsbDevice& operator=(const UsbDevice&) = delete;

    // True when a USB device with this VID/PID is present, whatever driver it uses.
    [[nodiscard]] static bool IsPresent(std::uint16_t vid, std::uint16_t pid);
    // True when a WinUSB interface for this VID/PID is present right now. The registry GUIDs
    // outlive a driver change (Zadig leaves them behind), so only a live interface under one counts.
    [[nodiscard]] static bool HasWinUsbDriver(std::uint16_t vid, std::uint16_t pid);

    // Opens the first WinUSB device matching VID/PID and the first interrupt or bulk IN and OUT pipes
    // of its interface 0. The WinUSB driver must already be bound (for example with Zadig), and the
    // interface is found through the DeviceInterfaceGUIDs its INF wrote to the registry.
    [[nodiscard]] bool Open(std::uint16_t vid, std::uint16_t pid);
    void Close();
    [[nodiscard]] bool IsOpen() const { return m_winusb != nullptr; }

    // Reads one packet from the IN pipe within timeoutMs and stores its size in received.
    [[nodiscard]] IoResult Read(std::span<std::uint8_t> buffer, unsigned timeoutMs, std::size_t& received);
    // Writes one packet to the OUT pipe within timeoutMs.
    [[nodiscard]] IoResult Write(std::span<const std::uint8_t> data, unsigned timeoutMs);

    [[nodiscard]] std::size_t InMaxPacket() const { return m_inMax; }
    [[nodiscard]] std::size_t OutMaxPacket() const { return m_outMax; }

private:
    // Maps a failed transfer's error code to Timeout or Lost.
    IoResult Classify(DWORD error) const;
    // Sets the pipe transfer timeout, skipping the call when the cached value already matches.
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
