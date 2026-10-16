/*
** CrabeLoader
** File description:
** Implements DisneyBaseProtocol: wake, tag scan setup, tag list audit, UID, identity and
** progression reads, verified progression writes and LED control. No I/O, no threads.
** Wire format and constants are those of the engine's Gateway layer.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_protocol.hpp"
#include "infrastructure/portal/portal_crypto.hpp"

#include <algorithm>
#include <cstring>
#include <format>

namespace crabe::infrastructure::portal {

namespace {

constexpr std::uint8_t kFrameHeader = 0xFF;
constexpr std::uint8_t kResponseHeader = 0xAA;
constexpr std::uint8_t kTagChangeHeader = 0xAB;

constexpr std::uint8_t kCmdWake = 0x80;
constexpr std::uint8_t kCmdSetAllLeds = 0x95;
constexpr std::uint8_t kCmdReadTagList = 0xA1;
constexpr std::uint8_t kCmdReadDataBlock = 0xA2;
constexpr std::uint8_t kCmdWriteDataBlock = 0xA3;
constexpr std::uint8_t kCmdGetTagUid = 0xB4;
constexpr std::uint8_t kCmdSetTagScanInterval = 0xB5;

constexpr char kWakeString[] = "(c) Disney 2013";
constexpr std::size_t kWakeStringLen = sizeof(kWakeString) - 1;

constexpr std::uint8_t kIdSector = 0;
constexpr std::uint8_t kIdBlock = 1;

constexpr std::array<std::array<std::uint8_t, 2>, 4> kProgressSectorBlock{{{1, 0}, {2, 0}, {1, 1}, {2, 1}}};
constexpr std::size_t kExperienceCopy1 = 0;
constexpr std::size_t kExperienceCopy2 = 1;
constexpr std::size_t kSkillsCopy1 = 2;
constexpr std::size_t kSkillsCopy2 = 3;
constexpr std::size_t kSequenceOffset = 11;

constexpr std::uint8_t kLedMaskPlayset = 0x01;
constexpr std::uint8_t kLedMaskPlayer1 = 0x02;
constexpr std::uint8_t kLedMaskPlayer2 = 0x04;
constexpr std::uint8_t kLedMaskAll = kLedMaskPlayset | kLedMaskPlayer1 | kLedMaskPlayer2;
constexpr std::uint8_t kForcedScanInterval = 0xFF;

constexpr std::uint8_t kStatusOk = 0x00;
constexpr std::uint8_t kStatusTagNotFound = 0x80;

constexpr auto kWakeRetry = std::chrono::milliseconds(1000);
constexpr auto kScanResetWait = std::chrono::milliseconds(1000);
constexpr auto kTagListPeriod = std::chrono::milliseconds(3000);
constexpr auto kRequestTimeout = std::chrono::milliseconds(1500);
constexpr auto kRemovalGrace = std::chrono::milliseconds(1500);
constexpr std::uint8_t kMaxTries = 3;

std::uint32_t ReadBe32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24) | (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) | static_cast<std::uint32_t>(p[3]);
}

// The engine's CompareTimeStamps on the sequence bytes of two copies; true when the first copy is current.
// The sequence wraps from 0xFF to 0, two zeros keep the first copy, and any other tie keeps the second.
bool FirstCopyIsCurrent(std::uint8_t first, std::uint8_t second) {
    if (first == 0 && second == 0xFF) return true;
    if (first == 0xFF && second == 0) return false;
    if (first == 0 && second == 0) return true;
    return first > second;
}

// Returns the current copy of a pair, the only valid one, or an empty optional when neither is valid.
template <typename Block>
const std::optional<Block>& CurrentCopy(const std::optional<Block>& first, const std::optional<Block>& second) {
    if (!first) return second;
    if (!second) return first;
    return FirstCopyIsCurrent((*first)[kSequenceOffset], (*second)[kSequenceOffset]) ? first : second;
}

// Report checksum: the low byte of the sum of every preceding byte.
std::uint8_t Checksum(const std::uint8_t* data, std::size_t count) {
    unsigned sum = 0;
    for (std::size_t i = 0; i < count; ++i) sum += data[i];
    return static_cast<std::uint8_t>(sum & 0xFFU);
}

} // namespace

std::uint8_t DisneyBaseProtocol::SlotForAntenna(std::uint8_t antenna) {
    switch (antenna) {
        case 1: return 3;
        case 2: return 1;
        case 3: return 2;
        default: return 0;
    }
}

void DisneyBaseProtocol::Reset() {
    m_phase = Phase::Waking;
    m_info = {};
    m_pending = {};
    m_tags = {};
    m_outgoing.clear();
    m_events.clear();
    m_started = false;
}

void DisneyBaseProtocol::DropAllTags() {
    for (std::uint8_t lti = 0; lti < kMaxTags; ++lti) TagRemoved(lti);
}

bool DisneyBaseProtocol::PopOutgoing(Report& out) {
    if (m_outgoing.empty()) return false;
    out = m_outgoing.front();
    m_outgoing.pop_front();
    return true;
}

std::vector<PortalTagEvent> DisneyBaseProtocol::TakeEvents() {
    std::vector<PortalTagEvent> events;
    events.swap(m_events);
    return events;
}

void DisneyBaseProtocol::Send(std::uint8_t cmd, std::span<const std::uint8_t> data, Kind kind,
                              std::uint8_t lti, Clock::time_point now) {
    constexpr std::size_t kFramingBytes = 5;
    if (data.size() > kReportSize - kFramingBytes) return;

    const std::uint8_t id = ++m_nextId;
    Report report{};
    report[0] = kFrameHeader;
    report[1] = static_cast<std::uint8_t>(2 + data.size());
    report[2] = cmd;
    report[3] = id;
    std::copy(data.begin(), data.end(), report.begin() + 4);
    report[4 + data.size()] = Checksum(report.data(), 4 + data.size());

    m_pending[id] = Pending{kind, lti, 0, now};
    m_outgoing.push_back(report);
}

void DisneyBaseProtocol::SendWake(Clock::time_point now) {
    std::array<std::uint8_t, kWakeStringLen> text{};
    std::memcpy(text.data(), kWakeString, kWakeStringLen);
    Send(kCmdWake, text, Kind::Wake, 0, now);
    m_lastWake = now;
}

void DisneyBaseProtocol::SendGetUid(std::uint8_t lti, Clock::time_point now) {
    const std::array<std::uint8_t, 1> data{lti};
    Send(kCmdGetTagUid, data, Kind::Uid, lti, now);
}

void DisneyBaseProtocol::SendReadId(std::uint8_t lti, Clock::time_point now) {
    const std::array<std::uint8_t, 3> data{lti, kIdSector, kIdBlock};
    Send(kCmdReadDataBlock, data, Kind::ReadId, lti, now);
}

void DisneyBaseProtocol::SendReadProgress(std::uint8_t lti, std::uint8_t block, Clock::time_point now) {
    const std::array<std::uint8_t, 3> data{lti, kProgressSectorBlock[block][0], kProgressSectorBlock[block][1]};
    Send(kCmdReadDataBlock, data, Kind::ReadProgress, lti, now);
    m_pending[m_nextId].progressIndex = block;
}

void DisneyBaseProtocol::SetLed(std::uint8_t slot, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    std::uint8_t mask = 0;
    switch (slot) {
        case 0: mask = kLedMaskAll; break;
        case 1: mask = kLedMaskPlayer1; break;
        case 2: mask = kLedMaskPlayer2; break;
        case 3: mask = kLedMaskPlayset; break;
        default: return;
    }
    std::array<std::uint8_t, 12> data{};
    for (std::size_t led = 0; led < 3; ++led) {
        data[led * 4 + 0] = (mask & (1U << led)) ? 1 : 0;
        data[led * 4 + 1] = r;
        data[led * 4 + 2] = g;
        data[led * 4 + 3] = b;
    }
    Send(kCmdSetAllLeds, data, Kind::Led, 0, Clock::now());
}

void DisneyBaseProtocol::Tick(Clock::time_point now) {
    switch (m_phase) {
        case Phase::Waking:
            if (!m_started || now - m_lastWake >= kWakeRetry) {
                m_started = true;
                SendWake(now);
            }
            break;

        case Phase::ScanReset:
            if (now - m_phaseSince >= kScanResetWait) {
                const std::array<std::uint8_t, 1> interval{m_options.runningScanInterval};
                Send(kCmdSetTagScanInterval, interval, Kind::Scan, 0, now);
                m_phase = Phase::Running;
                m_phaseSince = now;
                m_lastTagList = now - kTagListPeriod;
            }
            break;

        case Phase::Running:
            if (now - m_lastTagList >= kTagListPeriod) {
                m_lastTagList = now;
                Send(kCmdReadTagList, {}, Kind::TagList, 0, now);
            }
            break;
    }

    for (std::size_t id = 0; id < m_pending.size(); ++id) {
        Pending& p = m_pending[id];
        if (p.kind == Kind::None || now - p.sentAt < kRequestTimeout) continue;
        const Kind kind = p.kind;
        const std::uint8_t lti = p.lti;
        const std::uint8_t block = p.progressIndex;
        p = {};
        if (kind == Kind::ReadProgress && lti < kMaxTags) {
            ProgressBlockDone(lti, block, std::nullopt);
            continue;
        }
        if ((kind == Kind::WriteBlock || kind == Kind::VerifyBlock) && lti < kMaxTags) {
            WriteBlockDone(lti, block, false, "no answer from the base");
            continue;
        }
        if ((kind == Kind::Uid || kind == Kind::ReadId) && lti < kMaxTags && m_tags[lti].present &&
            !m_tags[lti].reported && m_tags[lti].readTries < kMaxTries) {
            ++m_tags[lti].readTries;
            if (kind == Kind::Uid) SendGetUid(lti, now);
            else SendReadId(lti, now);
        }
    }
}

void DisneyBaseProtocol::OnReport(std::span<const std::uint8_t> report, Clock::time_point now) {
    if (report.size() < 4) return;
    const std::uint8_t header = report[0];
    if (header != kResponseHeader && header != kTagChangeHeader) return;

    const std::size_t payload = report[1];
    if (payload + 3 > report.size()) return;
    if (Checksum(report.data(), payload + 2) != report[payload + 2]) {
        Log("portal: report dropped (bad checksum)");
        return;
    }

    if (header == kResponseHeader) HandleResponse(report, now);
    else HandleTagChange(report, now);
}

void DisneyBaseProtocol::HandleResponse(std::span<const std::uint8_t> r, Clock::time_point now) {
    const std::uint8_t id = r[2];
    const Pending pending = m_pending[id];
    if (pending.kind == Kind::None) return;
    m_pending[id] = {};

    switch (pending.kind) {
        case Kind::Wake:
            if (r.size() < 12 || m_phase != Phase::Waking) return;
            m_info.platformId = static_cast<std::uint16_t>((r[3] << 8) | r[4]);
            m_info.hardwareMajor = r[5];
            m_info.hardwareMinor = r[6];
            m_info.protocolMajor = r[7];
            m_info.protocolMinor = r[8];
            m_info.firmwareMajor = r[9];
            m_info.firmwareMinor = r[10];
            Log(std::format("portal: base awake (platform 0x{:04x}, hw {}.{}, protocol {}.{}, firmware {}.{})",
                            m_info.platformId, m_info.hardwareMajor, m_info.hardwareMinor,
                            m_info.protocolMajor, m_info.protocolMinor, m_info.firmwareMajor,
                            m_info.firmwareMinor));
            {
                const std::array<std::uint8_t, 1> forcedScan{kForcedScanInterval};
                Send(kCmdSetTagScanInterval, forcedScan, Kind::Scan, 0, now);
            }
            m_phase = Phase::ScanReset;
            m_phaseSince = now;
            break;
        case Kind::TagList: HandleTagList(r, now); break;
        case Kind::Uid: HandleUid(pending.lti, r, now); break;
        case Kind::ReadId: HandleReadId(pending.lti, r, now); break;
        case Kind::ReadProgress: HandleReadProgress(pending.lti, pending.progressIndex, r); break;
        case Kind::WriteBlock: HandleWriteBlock(pending.lti, pending.progressIndex, r, now); break;
        case Kind::VerifyBlock: HandleVerifyBlock(pending.lti, pending.progressIndex, r); break;
        default: break;
    }
}

void DisneyBaseProtocol::HandleTagChange(std::span<const std::uint8_t> r, Clock::time_point now) {
    if (r.size() < 7) return;
    const std::uint8_t antenna = r[2];
    const std::uint8_t type = r[3];
    const std::uint8_t lti = r[4];
    const bool inserted = r[5] == 0;
    if (lti >= kMaxTags) return;
    if (inserted) TagInserted(lti, antenna, type, now);
    else TagRemoved(lti);
}

void DisneyBaseProtocol::HandleTagList(std::span<const std::uint8_t> r, Clock::time_point now) {
    const std::size_t payload = r[1];
    const std::size_t count = payload > 0 ? (payload - 1) / 2 : 0;
    std::array<bool, kMaxTags> seen{};

    for (std::size_t i = 0; i < count && 4 + 2 * i < r.size(); ++i) {
        const std::uint8_t antennaAndLti = r[3 + 2 * i];
        const std::uint8_t lti = antennaAndLti & 0x0F;
        const std::uint8_t antenna = antennaAndLti >> 4;
        const std::uint8_t type = r[4 + 2 * i];
        if (lti >= kMaxTags) continue;
        seen[lti] = true;
        if (!m_tags[lti].present) TagInserted(lti, antenna, type, now);
    }

    for (std::uint8_t lti = 0; lti < kMaxTags; ++lti) {
        if (m_tags[lti].present && !seen[lti] && now - m_tags[lti].since > kRemovalGrace) TagRemoved(lti);
    }
}

void DisneyBaseProtocol::HandleUid(std::uint8_t lti, std::span<const std::uint8_t> r, Clock::time_point now) {
    if (lti >= kMaxTags || !m_tags[lti].present || r.size() < 11) return;
    Tag& tag = m_tags[lti];

    const std::uint8_t status = r[3];
    if (status == kStatusOk) {
        std::uint64_t uid = 0;
        for (std::size_t i = 0; i < 7; ++i) {
            tag.uidBytes[i] = r[4 + i];
            uid = (uid << 8) | r[4 + i];
        }
        tag.uid = uid;
        SendReadId(lti, now);
    } else if (status == kStatusTagNotFound) {
        tag = {};
    } else if (tag.readTries < kMaxTries) {
        ++tag.readTries;
        SendGetUid(lti, now);
    }
}

void DisneyBaseProtocol::HandleReadId(std::uint8_t lti, std::span<const std::uint8_t> r, Clock::time_point now) {
    if (lti >= kMaxTags || !m_tags[lti].present || r.size() < 20) return;
    Tag& tag = m_tags[lti];

    if (r[3] != kStatusOk) {
        if (tag.readTries < kMaxTries) {
            ++tag.readTries;
            SendReadId(lti, now);
        } else {
            Log(std::format("portal: identity block read failed on tag {} (status 0x{:02x})", lti, r[3]));
        }
        return;
    }

    std::array<std::uint8_t, 16> encrypted{};
    std::copy_n(r.begin() + 4, 16, encrypted.begin());
    const auto id = DecodeIdBlock(std::span<const std::uint8_t, 7>(tag.uidBytes), encrypted);
    if (!id) {
        Log(std::format("portal: tag {:014x} is not an Infinity figure (identity magic mismatch)", tag.uid));
        return;
    }

    tag.sku = id->sku;
    Log(std::format("portal: figure sku {} on antenna {} (uid {:014x}, id crc {})", id->sku, tag.antenna,
                    tag.uid, id->crcValid ? "ok" : "mismatch"));

    tag.progressRemaining = static_cast<std::uint8_t>(kProgressBlockCount);
    for (std::uint8_t block = 0; block < kProgressBlockCount; ++block) SendReadProgress(lti, block, now);
}

void DisneyBaseProtocol::HandleReadProgress(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r) {
    if (lti >= kMaxTags || block >= kProgressBlockCount) return;
    std::optional<DataBlock> data;
    if (r.size() >= 20 && r[3] == kStatusOk) {
        std::array<std::uint8_t, 16> encrypted{};
        std::copy_n(r.begin() + 4, 16, encrypted.begin());
        data = DecryptDataBlock(std::span<const std::uint8_t, 7>(m_tags[lti].uidBytes), encrypted);
    }
    ProgressBlockDone(lti, block, data);
}

void DisneyBaseProtocol::ProgressBlockDone(std::uint8_t lti, std::uint8_t block, std::optional<DataBlock> data) {
    Tag& tag = m_tags[lti];
    if (!tag.present || tag.reported || tag.progressRemaining == 0) return;
    tag.progressBlocks[block] = data;
    if (--tag.progressRemaining == 0) ReportPlaced(lti);
}

void DisneyBaseProtocol::ReportPlaced(std::uint8_t lti) {
    Tag& tag = m_tags[lti];
    tag.reported = true;

    PortalTagEvent ev;
    ev.type = PortalTagEvent::Type::Placed;
    ev.slot = SlotForAntenna(tag.antenna);
    ev.sku = tag.sku;
    ev.uid = tag.uid;

    const auto& experience = CurrentCopy(tag.progressBlocks[kExperienceCopy1], tag.progressBlocks[kExperienceCopy2]);
    if (experience) {
        const DataBlock& e = *experience;
        FigureProgress progress;
        progress.experience = ReadBe32(&e[0]);
        progress.level = e[4];
        progress.lastPlayed = ReadBe32(&e[5]);
        progress.actionFlags = e[9];
        if (const auto& skills = CurrentCopy(tag.progressBlocks[kSkillsCopy1], tag.progressBlocks[kSkillsCopy2])) {
            std::copy_n(skills->begin(), progress.skillBits.size(), progress.skillBits.begin());
        }
        Log(std::format("portal: figure sku {} saved progress: level {}, xp {}", tag.sku, progress.level,
                        progress.experience));
        ev.progress = progress;
    }
    if (ev.slot != 0) m_events.push_back(ev);
}

bool DisneyBaseProtocol::WriteProgress(std::uint8_t slot, const FigureProgress& progress) {
    std::uint8_t lti = kMaxTags;
    for (std::uint8_t i = 0; i < kMaxTags; ++i) {
        if (m_tags[i].present && m_tags[i].reported && SlotForAntenna(m_tags[i].antenna) == slot) lti = i;
    }
    if (lti == kMaxTags) return false;
    Tag& tag = m_tags[lti];
    if (tag.writesRemaining != 0) return false;

    const auto sealIntoOlderCopy = [&](std::size_t first, std::size_t second, const DataBlock& content) {
        const auto& a = tag.progressBlocks[first];
        const auto& b = tag.progressBlocks[second];
        if (!a && !b) return;
        const bool firstCurrent = a && (!b || FirstCopyIsCurrent((*a)[kSequenceOffset], (*b)[kSequenceOffset]));
        const DataBlock& current = firstCurrent ? *a : *b;
        const bool unchanged = std::equal(content.begin(), content.begin() + kSequenceOffset, current.begin());
        if (unchanged) return;
        DataBlock next = content;
        next[kSequenceOffset] = static_cast<std::uint8_t>(current[kSequenceOffset] + 1);
        const std::size_t target = firstCurrent ? second : first;
        const auto sealed = SealDataBlock(std::span<const std::uint8_t, 7>(tag.uidBytes), next);
        if (!sealed) return;
        tag.writeSealed[target] = *sealed;
        ++tag.writesRemaining;
    };

    const auto& currentExperience = CurrentCopy(tag.progressBlocks[kExperienceCopy1], tag.progressBlocks[kExperienceCopy2]);
    const auto& currentSkills = CurrentCopy(tag.progressBlocks[kSkillsCopy1], tag.progressBlocks[kSkillsCopy2]);

    DataBlock experience{};
    for (std::size_t i = 0; i < 4; ++i) {
        experience[i] = static_cast<std::uint8_t>(progress.experience >> (24 - 8 * i));
        experience[5 + i] = static_cast<std::uint8_t>(progress.lastPlayed >> (24 - 8 * i));
    }
    experience[4] = progress.level;
    experience[9] = progress.actionFlags;
    if (currentExperience) experience[10] = (*currentExperience)[10];
    sealIntoOlderCopy(kExperienceCopy1, kExperienceCopy2, experience);

    DataBlock skills{};
    std::copy(progress.skillBits.begin(), progress.skillBits.end(), skills.begin());
    if (currentSkills) std::copy_n(currentSkills->begin() + 8, 3, skills.begin() + 8);
    sealIntoOlderCopy(kSkillsCopy1, kSkillsCopy2, skills);

    if (tag.writesRemaining == 0) return false;
    const auto now = Clock::now();
    for (std::uint8_t block = 0; block < kProgressBlockCount; ++block) {
        if (tag.writeSealed[block]) SendWriteBlock(lti, block, now);
    }
    Log(std::format("portal: saving progress on figure sku {} (level {}, xp {})", tag.sku, progress.level,
                    progress.experience));
    return true;
}

void DisneyBaseProtocol::SendWriteBlock(std::uint8_t lti, std::uint8_t block, Clock::time_point now) {
    const auto [sector, index] = kProgressSectorBlock[block];
    if (sector < 1 || sector > 2 || index > 1 || !m_tags[lti].writeSealed[block]) return;
    std::array<std::uint8_t, 19> data{lti, sector, index};
    std::copy(m_tags[lti].writeSealed[block]->begin(), m_tags[lti].writeSealed[block]->end(), data.begin() + 3);
    Send(kCmdWriteDataBlock, data, Kind::WriteBlock, lti, now);
    m_pending[m_nextId].progressIndex = block;
}

void DisneyBaseProtocol::HandleWriteBlock(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r,
                                          Clock::time_point now) {
    if (lti >= kMaxTags || block >= kProgressBlockCount) return;
    if (r.size() < 4 || r[3] != kStatusOk) {
        WriteBlockDone(lti, block, false, std::format("write refused (status 0x{:02x})", r.size() > 3 ? r[3] : 0));
        return;
    }
    const std::array<std::uint8_t, 3> data{lti, kProgressSectorBlock[block][0], kProgressSectorBlock[block][1]};
    Send(kCmdReadDataBlock, data, Kind::VerifyBlock, lti, now);
    m_pending[m_nextId].progressIndex = block;
}

void DisneyBaseProtocol::HandleVerifyBlock(std::uint8_t lti, std::uint8_t block, std::span<const std::uint8_t> r) {
    if (lti >= kMaxTags || block >= kProgressBlockCount) return;
    Tag& tag = m_tags[lti];
    if (!tag.writeSealed[block]) return;
    if (r.size() < 20 || r[3] != kStatusOk || !std::equal(tag.writeSealed[block]->begin(), tag.writeSealed[block]->end(), r.begin() + 4)) {
        WriteBlockDone(lti, block, false, "read-back differs from what was written");
        return;
    }
    tag.progressBlocks[block] = DecryptDataBlock(std::span<const std::uint8_t, 7>(tag.uidBytes), *tag.writeSealed[block]);
    WriteBlockDone(lti, block, true, {});
}

void DisneyBaseProtocol::WriteBlockDone(std::uint8_t lti, std::uint8_t block, bool ok, std::string_view why) {
    Tag& tag = m_tags[lti];
    if (!tag.present || !tag.writeSealed[block]) return;
    tag.writeSealed[block].reset();
    if (tag.writesRemaining > 0) --tag.writesRemaining;
    const auto [sector, index] = kProgressSectorBlock[block];
    if (ok) {
        Log(std::format("portal: figure sku {}: sector {} block {} written and verified", tag.sku, sector, index));
    } else {
        Log(std::format("portal: figure sku {}: sector {} block {} NOT saved ({}); the other copy is intact",
                        tag.sku, sector, index, why));
    }
}

void DisneyBaseProtocol::TagInserted(std::uint8_t lti, std::uint8_t antenna, std::uint8_t type,
                                     Clock::time_point now) {
    Tag& tag = m_tags[lti];
    const bool duplicateNotification = tag.present && tag.antenna == antenna;
    if (duplicateNotification) return;

    tag = {};
    tag.present = true;
    tag.idRequested = true;
    tag.antenna = antenna;
    tag.type = type;
    tag.since = now;
    SendGetUid(lti, now);
}

void DisneyBaseProtocol::TagRemoved(std::uint8_t lti) {
    Tag& tag = m_tags[lti];
    if (!tag.present) return;
    if (tag.reported) {
        const std::uint8_t slot = SlotForAntenna(tag.antenna);
        if (slot != 0) {
            PortalTagEvent ev;
            ev.type = PortalTagEvent::Type::Removed;
            ev.slot = slot;
            ev.sku = tag.sku;
            ev.uid = tag.uid;
            m_events.push_back(ev);
        }
        Log(std::format("portal: figure sku {} removed from antenna {}", tag.sku, tag.antenna));
    }
    tag = {};
}

} // namespace crabe::infrastructure::portal
