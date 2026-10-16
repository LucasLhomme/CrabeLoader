/*
** CrabeLoader
** File description:
** Offline checks of the progression write path against a simulated base holding Joy's real blocks.
** Check 1: SealDataBlock turns each real decrypted block back into exactly the bytes on the tag.
** Check 2: WriteProgress rewrites only the older copy of each changed pair, sequence advanced.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_crypto.hpp"
#include "infrastructure/portal/portal_protocol.hpp"

#include <array>
#include <cstdio>
#include <map>
#include <string>
#include <utility>

using namespace crabe::infrastructure::portal;
using Block = std::array<std::uint8_t, 16>;

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    std::printf("%s %s\n", condition ? "[OK]  " : "[FAIL]", what);
    if (!condition) ++g_failures;
}

Block FromHex(const char* hex) {
    Block b{};
    for (std::size_t i = 0; i < 16; ++i) {
        unsigned v = 0;
        std::sscanf(hex + 2 * i, "%2x", &v);
        b[i] = static_cast<std::uint8_t>(v);
    }
    return b;
}

constexpr std::array<std::uint8_t, 7> kUid{0x04, 0x68, 0x5c, 0x82, 0x17, 0x3f, 0x80};

constexpr std::uint8_t kCmdWake = 0x80;
constexpr std::uint8_t kCmdReadTagList = 0xA1;
constexpr std::uint8_t kCmdReadDataBlock = 0xA2;
constexpr std::uint8_t kCmdWriteDataBlock = 0xA3;
constexpr std::uint8_t kCmdGetTagUid = 0xB4;

// Raw tag content keyed by (sector, block), dumped read-only from figure_04685c82173f80_20261014_141128.json.
std::map<std::pair<int, int>, Block> RealTag() {
    return {
        {{0, 1}, FromHex("0cf93ac2130f96cf38b7cb81a05619e5")},
        {{1, 0}, FromHex("892edd4b406823215d19b84d1544be8e")},
        {{1, 1}, FromHex("d4b1596f1613fd1cba90168a1073e4fa")},
        {{2, 0}, FromHex("fab01797e376211c9ff4738eeb1eca5c")},
        {{2, 1}, FromHex("e8f26e7812797b4340dad67fbe60d24a")},
    };
}

class FakeBase {
public:
    std::map<std::pair<int, int>, Block> tag = RealTag();
    std::vector<std::pair<int, int>> writes;

    // Answers one request like a base holding the figure as LTI 0 on antenna 2 (Player 1).
    // Block writes update the tag map and are logged in writes; unknown blocks read back status 0x01.
    std::vector<Report> Answer(const Report& request) {
        const std::uint8_t cmd = request[2];
        const std::uint8_t id = request[3];
        std::vector<std::uint8_t> payload;
        switch (cmd) {
            case kCmdWake: payload = {0x02, 0x00, 0x02, 0x00, 0x03, 0x04, 0x0C, 0x07}; break;
            case kCmdReadTagList: payload = {0x20, 0x09}; break;
            case kCmdGetTagUid: payload = {0x00}; payload.insert(payload.end(), kUid.begin(), kUid.end()); break;
            case kCmdReadDataBlock: {
                const auto it = tag.find({request[5], request[6]});
                payload = {static_cast<std::uint8_t>(it == tag.end() ? 0x01 : 0x00)};
                const Block block = it == tag.end() ? Block{} : it->second;
                payload.insert(payload.end(), block.begin(), block.end());
                break;
            }
            case kCmdWriteDataBlock: {
                Block block{};
                std::copy_n(request.begin() + 7, 16, block.begin());
                tag[{request[5], request[6]}] = block;
                writes.emplace_back(request[5], request[6]);
                payload = {0x00};
                break;
            }
            default: payload = {}; break;
        }
        Report r{};
        r[0] = 0xAA;
        r[1] = static_cast<std::uint8_t>(1 + payload.size());
        r[2] = id;
        std::copy(payload.begin(), payload.end(), r.begin() + 3);
        unsigned sum = 0;
        for (std::size_t i = 0; i < 3 + payload.size(); ++i) sum += r[i];
        r[3 + payload.size()] = static_cast<std::uint8_t>(sum);
        return {r};
    }
};

// Runs the protocol against the fake base for the given number of 100 ms ticks, answering every request.
void Pump(DisneyBaseProtocol& protocol, FakeBase& base, std::chrono::steady_clock::time_point& now, int rounds) {
    for (int i = 0; i < rounds; ++i) {
        now += std::chrono::milliseconds(100);
        protocol.Tick(now);
        Report out;
        while (protocol.PopOutgoing(out)) {
            for (const Report& in : base.Answer(out)) protocol.OnReport(in, now);
        }
    }
}

} // namespace

int main() {
    const auto uid = std::span<const std::uint8_t, 7>(kUid);

    for (const auto& [where, raw] : RealTag()) {
        if (where.first == 0) continue;
        const auto plain = DecryptDataBlock(uid, raw);
        Check(plain.has_value(), ("decrypt + CRC S" + std::to_string(where.first) + "B" + std::to_string(where.second)).c_str());
        if (!plain) continue;
        const auto sealed = SealDataBlock(uid, *plain);
        Check(sealed && *sealed == raw, ("seal == tag bytes S" + std::to_string(where.first) + "B" + std::to_string(where.second)).c_str());
    }

    FakeBase base;
    DisneyBaseProtocol protocol(DisneyBaseProtocol::Options{3});
    auto now = std::chrono::steady_clock::now();
    Pump(protocol, base, now, 40);
    auto events = protocol.TakeEvents();
    Check(events.size() == 1 && events[0].progress.has_value(), "figure placed with saved progress");
    if (events.empty() || !events[0].progress) return 1;
    FigureProgress progress = *events[0].progress;
    Check(progress.level == 9 && progress.experience == 102, "current copy = sector 1 (level 9, xp 102)");

    const auto before = base.tag;
    progress.level = 10;
    progress.experience = 0;
    progress.skillBits[0] |= 0x02;
    Check(protocol.WriteProgress(1, progress), "write queued");
    Pump(protocol, base, now, 20);

    Check(base.writes.size() == 2, "exactly two blocks written");
    bool onlyOlderCopies = true;
    for (const auto& w : base.writes) onlyOlderCopies &= (w == std::pair<int, int>{2, 0} || w == std::pair<int, int>{2, 1});
    Check(onlyOlderCopies, "only the older copies (sector 2) were written");
    Check(base.tag[{1, 0}] == before.at({1, 0}) && base.tag[{1, 1}] == before.at({1, 1}) && base.tag[{0, 1}] == before.at({0, 1}),
          "identity and current copies untouched");

    const auto exp = DecryptDataBlock(uid, base.tag[{2, 0}]);
    Check(exp && (*exp)[4] == 10 && (*exp)[11] == 0x49, "new experience copy: level 10, sequence 0x48 + 1");
    const auto skills = DecryptDataBlock(uid, base.tag[{2, 1}]);
    Check(skills && (*skills)[0] == 0x03 && (*skills)[11] == 0x04, "new skills copy: extra bit, sequence 0x03 + 1");

    protocol.DropAllTags();
    static_cast<void>(protocol.TakeEvents());
    DisneyBaseProtocol reader(DisneyBaseProtocol::Options{3});
    Pump(reader, base, now, 40);
    events = reader.TakeEvents();
    Check(events.size() == 1 && events[0].progress && events[0].progress->level == 10 &&
              events[0].progress->skillBits[0] == 0x03,
          "re-read picks the written copies");

    base.writes.clear();
    Check(!reader.WriteProgress(1, *events[0].progress) && base.writes.empty(), "unchanged progress is not rewritten");

    std::printf("%s\n", g_failures == 0 ? "ALL OK" : "FAILURES");
    return g_failures == 0 ? 0 : 1;
}
