/*
** CrabeLoader
** File description:
** Declares DisneyBaseProtocol, the transport-agnostic Disney Infinity base protocol.
** It owns no I/O and no thread: the owner feeds it incoming reports, drains the reports
** it wants sent, and calls Tick() with the clock.
**
** Authors: @LucasLhomme
*/

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

struct BaseInfo {
    std::uint16_t platformId{0};
    std::uint8_t hardwareMajor{0}, hardwareMinor{0};
    std::uint8_t protocolMajor{0}, protocolMinor{0};
    std::uint8_t firmwareMajor{0}, firmwareMinor{0};
};

class DisneyBaseProtocol final {
public:
    using LogSink = std::function<void(std::string_view)>;

    struct Options {
        std::uint8_t runningScanInterval{1};
    };

    DisneyBaseProtocol() = default;
    explicit DisneyBaseProtocol(Options options) : m_options(options) {}

    void SetLogSink(LogSink sink) { m_log = std::move(sink); }
    void Reset();

    // Feeds one report from the base: [0xAA, len, id, data..., checksum] answers a pending request
    // and [0xAB, 4, antenna, type, lti, 0 = inserted, checksum] reports a tag change. Reports shorter
    // than 4 bytes, with another header or with a bad checksum are dropped.
    void OnReport(std::span<const std::uint8_t> report, std::chrono::steady_clock::time_point now);

    // Advances timers: wake retries, the scan-interval reset, the periodic tag list and request timeouts.
    // After the wake a forced 0xFF scan interval resets the firmware tag timer, which can wrap negative
    // and stop scanning, before the running interval is set; unanswered requests are retried or dropped.
    void Tick(std::chrono::steady_clock::time_point now);

    // Pops the next report to send to the base. False when the queue is empty.
    [[nodiscard]] bool PopOutgoing(Report& out);

    // Hands over and clears the figure events detected so far.
    [[nodiscard]] std::vector<PortalTagEvent> TakeEvents();

    // Queues a Removed event for every figure currently reported (the base went away).
    void DropAllTags();

    // Saves progression onto the figure of a player slot the way the engine does: each changed pair
    // (experience, skill bits) is sealed into its older copy with the sequence advanced, then read back
    // and compared. Only sectors 1-2, blocks 0-1 are ever written; returns false when nothing was queued
    // (no figure, no valid copy to build on, nothing changed, or a write already in flight).
    bool WriteProgress(std::uint8_t slot, const FigureProgress& progress);

    // Sets the LED colour of a slot: 0 = all, 1 = Player 1, 2 = Player 2, 3 = Playset / hexagon.
    // Other slot values are ignored.
    void SetLed(std::uint8_t slot, std::uint8_t r, std::uint8_t g, std::uint8_t b);

    [[nodiscard]] bool IsAwake() const { return m_phase == Phase::Running || m_phase == Phase::ScanReset; }
    [[nodiscard]] const BaseInfo& Info() const { return m_info; }

    // Maps the engine's antenna number to the Lua-facing slot: antenna 1 is the Playset (3), antenna 2
    // is Player 1 (1) and antenna 3 is Player 2 (2). Any other antenna gives 0.
    [[nodiscard]] static std::uint8_t SlotForAntenna(std::uint8_t antenna);

private:
    using Clock = std::chrono::steady_clock;

    enum class Phase { Waking, ScanReset, Running };
    enum class Kind : std::uint8_t { None, Wake, Scan, Led, TagList, Uid, ReadId, ReadProgress, WriteBlock, VerifyBlock };

    struct Pending {
        Kind kind{Kind::None};
        std::uint8_t lti{0};
        std::uint8_t progressIndex{0};
        Clock::time_point sentAt{};
    };

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
        std::array<std::optional<DataBlock>, kProgressBlockCount> writeSealed{};
        std::uint8_t writesRemaining{0};
        Clock::time_point since{};
    };

    static constexpr std::size_t kMaxTags = 16;

    // Frames a request as [0xFF, len, cmd, id, data..., checksum] in one report, records it as pending
    // under the next id and queues it. Data longer than 27 bytes is dropped without sending.
    void Send(std::uint8_t cmd, std::span<const std::uint8_t> data, Kind kind, std::uint8_t lti,
              Clock::time_point now);
    void SendWake(Clock::time_point now);
    void SendGetUid(std::uint8_t lti, Clock::time_point now);
    void SendReadId(std::uint8_t lti, Clock::time_point now);

    // Requests one progression block (an index into the progression block table) and stores that
    // index in the pending slot Send() just filled.
    void SendReadProgress(std::uint8_t lti, std::uint8_t block, Clock::time_point now);

    void HandleResponse(std::span<const std::uint8_t> r, Clock::time_point now);
    void HandleTagChange(std::span<const std::uint8_t> r, Clock::time_point now);

    // Inserts listed tags that were missed and removes, after a grace period, tags the base no longer
    // lists, which covers lost change notifications. Each entry is (antenna << 4 | lti) then the type.
    void HandleTagList(std::span<const std::uint8_t> r, Clock::time_point now);

    // Stores the 7-byte UID and requests the identity block. A tag-not-found status means the figure
    // left before the request was served; other errors retry up to the try limit.
    void HandleUid(std::uint8_t lti, std::span<const std::uint8_t> r, Clock::time_point now);

    // Decodes the identity block for the SKU, then requests the four progression blocks.
    // Non-character pieces have no valid progression copy and are reported without progress.
    void HandleReadId(std::uint8_t lti, std::span<const std::uint8_t> r, Clock::time_point now);
    void HandleReadProgress(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r);
    void ProgressBlockDone(std::uint8_t lti, std::uint8_t block, std::optional<DataBlock> data);

    // Queues the Placed event, with progression built from the current copy of each pair when one is valid.
    void ReportPlaced(std::uint8_t lti);

    // Sends the sealed bytes of one progression block to the base. Anything outside sectors 1-2,
    // blocks 0-1 is refused as a hard safety limit.
    void SendWriteBlock(std::uint8_t lti, std::uint8_t block, Clock::time_point now);

    // Reads the block back after an accepted write; a refused write ends that block's write.
    void HandleWriteBlock(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r, Clock::time_point now);

    // Compares the read-back bytes with the sealed bytes and, on a match, keeps the new copy as current data.
    void HandleVerifyBlock(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r);

    // Ends one block write and logs the outcome; a failed write leaves the other copy intact.
    void WriteBlockDone(std::uint8_t lti, std::uint8_t block, bool ok, std::string_view why);

    // Starts tracking a tag and requests its UID; a repeated notification for the same antenna is ignored.
    void TagInserted(std::uint8_t lti, std::uint8_t antenna, std::uint8_t type, Clock::time_point now);

    // Forgets a tag and queues a Removed event when its Placed event had been reported.
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
