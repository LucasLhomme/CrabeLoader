#pragma once

#include "infrastructure/portal/portal_types.hpp"

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace crabe::infrastructure::portal {

// Facts the base reports about itself in its wake response.
struct BaseInfo {
    std::uint16_t platformId{0};
    std::uint8_t hardwareMajor{0}, hardwareMinor{0};
    std::uint8_t protocolMajor{0}, protocolMinor{0};
    std::uint8_t firmwareMajor{0}, firmwareMinor{0};
};

// Transport-agnostic Disney Infinity base protocol (the "Gateway" protocol of the engine).
// Every request is a 32-byte report [0xFF, len, cmd, id, data..., checksum]; every answer is
// [0xAA, len, id, data..., checksum] and tag changes arrive unsolicited as [0xAB, 4, antenna,
// type, lti, insert/remove, checksum]. This class owns no I/O and no thread: the owner feeds
// it incoming reports, drains the reports it wants sent, and calls Tick() with the clock.
class DisneyBaseProtocol final {
public:
    using LogSink = std::function<void(std::string_view)>;

    struct Options {
        std::uint8_t runningScanInterval{1}; // the engine uses 3 on Xbox One, 1 elsewhere
    };

    DisneyBaseProtocol() = default;
    explicit DisneyBaseProtocol(Options options) : m_options(options) {}

    void SetLogSink(LogSink sink) { m_log = std::move(sink); }
    void Reset();

    // Feeds one report received from the base (at least 4 bytes; shorter ones are ignored).
    void OnReport(std::span<const std::uint8_t> report, std::chrono::steady_clock::time_point now);
    // Advances timers: wake retries, scan-interval reset, periodic tag list, request timeouts.
    void Tick(std::chrono::steady_clock::time_point now);

    // Pops the next report to send to the base. False when the queue is empty.
    [[nodiscard]] bool PopOutgoing(Report& out);
    // Hands over and clears the figure events detected so far.
    [[nodiscard]] std::vector<PortalTagEvent> TakeEvents();

    // Queues a Removed event for every figure currently reported (the base went away).
    void DropAllTags();

    // Saves progression onto the figure of a player slot the way the engine does: each changed
    // pair (experience, skill bits) goes to its older copy with the sequence advanced, then the
    // block is read back and compared. Only sectors 1-2, blocks 0-1 are ever written. False when
    // nothing could be queued (no figure, no valid saved copy to build on, write in progress).
    bool WriteProgress(std::uint8_t slot, const FigureProgress& progress);

    // slot: 0 = all LEDs, 1 = Player 1, 2 = Player 2, 3 = Playset / hexagon.
    void SetLed(std::uint8_t slot, std::uint8_t r, std::uint8_t g, std::uint8_t b);

    [[nodiscard]] bool IsAwake() const { return m_phase == Phase::Running || m_phase == Phase::ScanReset; }
    [[nodiscard]] const BaseInfo& Info() const { return m_info; }

    // The engine's antenna numbering mapped to the Lua-facing slot (1 = P1, 2 = P2, 3 = Playset).
    [[nodiscard]] static std::uint8_t SlotForAntenna(std::uint8_t antenna);

private:
    using Clock = std::chrono::steady_clock;

    enum class Phase { Waking, ScanReset, Running };
    enum class Kind : std::uint8_t { None, Wake, Scan, Led, TagList, Uid, ReadId, ReadProgress, WriteBlock, VerifyBlock };

    struct Pending {
        Kind kind{Kind::None};
        std::uint8_t lti{0};
        std::uint8_t block{0}; // ReadProgress: index into kProgressBlocks
        Clock::time_point sentAt{};
    };

    // Saved progression of a character figure, two copies each: experience in sector 1/2 block 0,
    // skill bits in sector 1/2 block 1. Read only.
    static constexpr std::size_t kProgressBlockCount = 4;
    using DataBlock = std::array<std::uint8_t, 16>;

    struct Tag {
        bool present{false};
        bool idRequested{false};
        bool reported{false};
        std::uint8_t antenna{0};
        std::uint8_t type{0};
        std::uint8_t readTries{0};
        std::uint64_t uid{0};
        std::array<std::uint8_t, 7> uidBytes{};
        std::uint32_t sku{0};
        std::array<std::optional<DataBlock>, kProgressBlockCount> progressBlocks{};
        std::uint8_t progressRemaining{0};
        // Write in flight: sealed bytes per progress block, compared with the read-back.
        std::array<std::optional<DataBlock>, kProgressBlockCount> writeSealed{};
        std::uint8_t writesRemaining{0};
        Clock::time_point since{};
    };

    static constexpr std::size_t kMaxTags = 16;

    void Send(std::uint8_t cmd, std::span<const std::uint8_t> data, Kind kind, std::uint8_t lti,
              Clock::time_point now);
    void SendWake(Clock::time_point now);
    void SendGetUid(std::uint8_t lti, Clock::time_point now);
    void SendReadId(std::uint8_t lti, Clock::time_point now);
    void SendReadProgress(std::uint8_t lti, std::uint8_t block, Clock::time_point now);

    void HandleResponse(std::span<const std::uint8_t> r, Clock::time_point now);
    void HandleTagChange(std::span<const std::uint8_t> r, Clock::time_point now);
    void HandleTagList(std::span<const std::uint8_t> r, Clock::time_point now);
    void HandleUid(std::uint8_t lti, std::span<const std::uint8_t> r, Clock::time_point now);
    void HandleReadId(std::uint8_t lti, std::span<const std::uint8_t> r, Clock::time_point now);
    void HandleReadProgress(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r);
    void ProgressBlockDone(std::uint8_t lti, std::uint8_t block, std::optional<DataBlock> data);
    void ReportPlaced(std::uint8_t lti);
    void SendWriteBlock(std::uint8_t lti, std::uint8_t block, Clock::time_point now);
    void HandleWriteBlock(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r, Clock::time_point now);
    void HandleVerifyBlock(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r);
    void WriteBlockDone(std::uint8_t lti, std::uint8_t block, bool ok, std::string_view why);

    void TagInserted(std::uint8_t lti, std::uint8_t antenna, std::uint8_t type, Clock::time_point now);
    void TagRemoved(std::uint8_t lti);

    void Log(std::string_view message) const {
        if (m_log) m_log(message);
    }

    Options m_options{};
    LogSink m_log;

    Phase m_phase{Phase::Waking};
    BaseInfo m_info{};
    std::uint8_t m_nextId{0};
    std::array<Pending, 256> m_pending{};
    std::array<Tag, kMaxTags> m_tags{};
    std::deque<Report> m_outgoing;
    std::vector<PortalTagEvent> m_events;

    Clock::time_point m_lastWake{};
    Clock::time_point m_phaseSince{};
    Clock::time_point m_lastTagList{};
    bool m_started{false};
};

} // namespace crabe::infrastructure::portal
