/*
** CrabeLoader
** File description:
** Xbox One Disney Infinity base (INF-8037059) transport: GIP over WinUSB.
** The session runs IDENTIFY, POWER ON, then the GIP v1 security handshake the base requires.
** The 32-byte Disney reports then travel as the payload of GIP message 0x21 in both directions.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/portal/portal_transport_factory.hpp"
#include "infrastructure/portal/usb_device.hpp"
#include "shared/logger.hpp"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace crabe::infrastructure::portal {

namespace {

using Bytes = std::vector<std::uint8_t>;
using Clock = std::chrono::steady_clock;

constexpr std::uint16_t kVid = 0x0E6F;
constexpr std::uint16_t kPid = 0x0229;

constexpr std::uint8_t kCmdAck = 0x01;
constexpr std::uint8_t kCmdIdentify = 0x04;
constexpr std::uint8_t kCmdPower = 0x05;
constexpr std::uint8_t kCmdAuth = 0x06;
constexpr std::uint8_t kCmdDisney = 0x21;
constexpr std::uint8_t kOptAck = 0x10;
constexpr std::uint8_t kOptInternal = 0x20;
constexpr std::uint8_t kOptChunkStart = 0x40;
constexpr std::uint8_t kOptChunk = 0x80;
constexpr std::size_t kChunkSize = 54;
constexpr std::size_t kPacketSize = 64;

constexpr std::uint8_t kAuthOptAck = 0x01;
constexpr std::uint8_t kAuthOptRequest = 0x02;
constexpr std::uint8_t kAuthOptFromHost = 0x40;
constexpr std::uint8_t kAuthHostHello = 0x01;
constexpr std::uint8_t kAuthClientHello = 0x02;
constexpr std::uint8_t kAuthCertificate = 0x03;
constexpr std::uint8_t kAuthHostSecret = 0x05;
constexpr std::uint8_t kAuthHostFinish = 0x07;
constexpr std::uint8_t kAuthClientFinish = 0x08;
constexpr std::size_t kAuthTrailer = 8;
constexpr std::size_t kRandomSize = 32;

constexpr unsigned kWriteTimeoutMs = 500;
constexpr auto kHandshakeDeadline = std::chrono::seconds(8);
constexpr auto kChunkAckWait = std::chrono::milliseconds(500);

class AlgHandle final {
public:
    AlgHandle(LPCWSTR id, ULONG flags) {
        if (BCryptOpenAlgorithmProvider(&m_handle, id, nullptr, flags) != 0) m_handle = nullptr;
    }
    ~AlgHandle() {
        if (m_handle) BCryptCloseAlgorithmProvider(m_handle, 0);
    }
    AlgHandle(const AlgHandle&) = delete;
    AlgHandle& operator=(const AlgHandle&) = delete;
    [[nodiscard]] BCRYPT_ALG_HANDLE get() const { return m_handle; }

private:
    BCRYPT_ALG_HANDLE m_handle{nullptr};
};

class KeyHandle final {
public:
    KeyHandle() = default;
    ~KeyHandle() {
        if (m_handle) BCryptDestroyKey(m_handle);
    }
    KeyHandle(const KeyHandle&) = delete;
    KeyHandle& operator=(const KeyHandle&) = delete;
    [[nodiscard]] BCRYPT_KEY_HANDLE* out() { return &m_handle; }
    [[nodiscard]] BCRYPT_KEY_HANDLE get() const { return m_handle; }

private:
    BCRYPT_KEY_HANDLE m_handle{nullptr};
};

bool Random(std::span<std::uint8_t> out) {
    return BCryptGenRandom(nullptr, out.data(), static_cast<ULONG>(out.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}

std::array<std::uint8_t, 32> Sha256(std::span<const std::uint8_t> data) {
    std::array<std::uint8_t, 32> digest{};
    AlgHandle sha(BCRYPT_SHA256_ALGORITHM, 0);
    if (sha.get()) {
        BCryptHash(sha.get(), nullptr, 0, const_cast<PUCHAR>(data.data()), static_cast<ULONG>(data.size()),
                   digest.data(), static_cast<ULONG>(digest.size()));
    }
    return digest;
}

std::array<std::uint8_t, 32> HmacSha256(std::span<const std::uint8_t> key, std::span<const std::uint8_t> data) {
    std::array<std::uint8_t, 32> mac{};
    AlgHandle hmac(BCRYPT_SHA256_ALGORITHM, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (hmac.get()) {
        BCryptHash(hmac.get(), const_cast<PUCHAR>(key.data()), static_cast<ULONG>(key.size()),
                   const_cast<PUCHAR>(data.data()), static_cast<ULONG>(data.size()), mac.data(),
                   static_cast<ULONG>(mac.size()));
    }
    return mac;
}

// TLS-style P_SHA256 over (label || seed), as the GIP handshake uses it.
Bytes Prf(std::span<const std::uint8_t> secret, std::string_view label, std::span<const std::uint8_t> seed,
          std::size_t size) {
    Bytes labelSeed(label.begin(), label.end());
    labelSeed.insert(labelSeed.end(), seed.begin(), seed.end());
    Bytes out;
    auto a = HmacSha256(secret, labelSeed);
    while (out.size() < size) {
        Bytes block(a.begin(), a.end());
        block.insert(block.end(), labelSeed.begin(), labelSeed.end());
        const auto chunk = HmacSha256(secret, block);
        out.insert(out.end(), chunk.begin(), chunk.end());
        a = HmacSha256(secret, a);
    }
    out.resize(size);
    return out;
}

// Encrypts message with PKCS#1 v1.5 padding under the RSA key of the accessory certificate.
// The key is a PKCS#1 RSAPublicKey (SEQUENCE 30 82 01 0A, 2048-bit INTEGER modulus, INTEGER exponent)
// that a normal X.509 parser cannot use, so it is located by its byte prefixes.
std::optional<Bytes> RsaPkcs1Encrypt(std::span<const std::uint8_t> certificate, std::span<const std::uint8_t> message) {
    static constexpr std::array<std::uint8_t, 4> kKeyPrefix{0x30, 0x82, 0x01, 0x0A};
    static constexpr std::array<std::uint8_t, 5> kModulusPrefix{0x02, 0x82, 0x01, 0x01, 0x00};
    constexpr std::size_t kModulusSize = 256;

    const auto key = std::search(certificate.begin(), certificate.end(), kKeyPrefix.begin(), kKeyPrefix.end());
    if (key == certificate.end()) return std::nullopt;
    const auto modulus = std::search(key, certificate.end(), kModulusPrefix.begin(), kModulusPrefix.end());
    const auto modulusStart = modulus + kModulusPrefix.size();
    if (modulus == certificate.end() || certificate.end() - modulusStart < static_cast<std::ptrdiff_t>(kModulusSize + 2)) {
        return std::nullopt;
    }
    const auto exponentHeader = modulusStart + kModulusSize;
    const std::size_t exponentSize = exponentHeader[1];
    if (exponentHeader[0] != 0x02 || certificate.end() - (exponentHeader + 2) < static_cast<std::ptrdiff_t>(exponentSize)) {
        return std::nullopt;
    }

    BCRYPT_RSAKEY_BLOB header{};
    header.Magic = BCRYPT_RSAPUBLIC_MAGIC;
    header.BitLength = kModulusSize * 8;
    header.cbPublicExp = static_cast<ULONG>(exponentSize);
    header.cbModulus = kModulusSize;
    Bytes blob(reinterpret_cast<const std::uint8_t*>(&header), reinterpret_cast<const std::uint8_t*>(&header) + sizeof(header));
    blob.insert(blob.end(), exponentHeader + 2, exponentHeader + 2 + exponentSize);
    blob.insert(blob.end(), modulusStart, modulusStart + kModulusSize);

    AlgHandle rsa(BCRYPT_RSA_ALGORITHM, 0);
    KeyHandle handle;
    if (!rsa.get() || BCryptImportKeyPair(rsa.get(), nullptr, BCRYPT_RSAPUBLIC_BLOB, handle.out(), blob.data(),
                                          static_cast<ULONG>(blob.size()), 0) != 0) {
        return std::nullopt;
    }
    Bytes out(kModulusSize);
    ULONG written = 0;
    if (BCryptEncrypt(handle.get(), const_cast<PUCHAR>(message.data()), static_cast<ULONG>(message.size()), nullptr,
                      nullptr, 0, out.data(), static_cast<ULONG>(out.size()), &written, BCRYPT_PAD_PKCS1) != 0) {
        return std::nullopt;
    }
    out.resize(written);
    return out;
}

void AppendBe16(Bytes& out, std::size_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value));
}

struct GipMessage {
    std::uint8_t command{0};
    std::uint8_t options{0};
    std::uint8_t sequence{0};
    Bytes payload;
};

std::size_t VarintSize(std::uint32_t value) {
    std::size_t size = 1;
    while (value >= 0x80) {
        value >>= 7;
        ++size;
    }
    return size;
}

void AppendVarint(Bytes& out, std::uint32_t value) {
    while (value >= 0x80) {
        out.push_back(static_cast<std::uint8_t>((value & 0x7F) | 0x80));
        value >>= 7;
    }
    out.push_back(static_cast<std::uint8_t>(value));
}

bool ReadVarint(std::span<const std::uint8_t> data, std::size_t& index, std::uint32_t& value) {
    value = 0;
    for (unsigned shift = 0; index < data.size() && shift < 28; shift += 7) {
        const std::uint8_t byte = data[index++];
        value |= static_cast<std::uint32_t>(byte & 0x7F) << shift;
        if (!(byte & 0x80)) return true;
    }
    return false;
}

// Header: command, options, sequence, varint length, [varint chunk offset]. Its total length must
// be even; when it is not, the length varint is padded with an extra continuation byte.
Bytes EncodePacket(std::uint8_t command, std::uint8_t options, std::uint8_t sequence,
                   std::span<const std::uint8_t> payload, std::uint32_t chunkOffset = 0) {
    const auto length = static_cast<std::uint32_t>(payload.size());
    const bool chunked = (options & kOptChunk) != 0;
    const std::size_t headerSize = 3 + VarintSize(length) + (chunked ? VarintSize(chunkOffset) : 0);
    Bytes out{command, options, sequence};
    AppendVarint(out, length);
    if (headerSize % 2) {
        out.back() |= 0x80;
        out.push_back(0);
    }
    if (chunked) AppendVarint(out, chunkOffset);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

class GipLink final {
public:
    explicit GipLink(UsbDevice& usb) : m_usb(usb) {}

    // Returns the next sequence number of this command, cycling through 1 to 255.
    std::uint8_t NextSequence(std::uint8_t command) {
        std::uint8_t& seq = m_sequences[command];
        seq = static_cast<std::uint8_t>(seq % 255 + 1);
        return seq;
    }

    bool Send(std::uint8_t command, std::uint8_t options, std::span<const std::uint8_t> payload) {
        const Bytes packet = EncodePacket(command, options, NextSequence(command), payload);
        return m_usb.Write(packet, kWriteTimeoutMs) == UsbDevice::IoResult::Ok;
    }

    // Sends an internal message longer than one packet: a first chunk carrying the total size, the
    // other chunks at their offset (each acknowledged by the device), then an empty terminator.
    bool SendChunked(std::uint8_t command, std::span<const std::uint8_t> message, std::vector<GipMessage>& received) {
        const std::uint8_t seq = NextSequence(command);
        const auto total = static_cast<std::uint32_t>(message.size());
        for (std::uint32_t offset = 0; offset < total;) {
            const std::size_t size = std::min<std::size_t>(kChunkSize, total - offset);
            const bool first = offset == 0;
            const std::uint8_t options = kOptChunk | kOptInternal | kOptAck | (first ? kOptChunkStart : 0);
            const Bytes packet = EncodePacket(command, options, seq, message.subspan(offset, size), first ? total : offset);
            if (m_usb.Write(packet, kWriteTimeoutMs) != UsbDevice::IoResult::Ok) return false;
            offset += static_cast<std::uint32_t>(size);
            if (!WaitAck(command, received)) return false;
        }
        const Bytes terminator = EncodePacket(command, kOptChunk | kOptInternal, seq, {}, total);
        return m_usb.Write(terminator, kWriteTimeoutMs) == UsbDevice::IoResult::Ok;
    }

    // Reads one USB packet, acknowledges it when asked, and reassembles chunked messages, whose first
    // chunk carries the total size in its offset field. Returns a complete message, device ACKs
    // included, when one is available.
    std::optional<GipMessage> Receive(unsigned timeoutMs, bool& lost) {
        lost = false;
        std::array<std::uint8_t, kPacketSize> packet{};
        std::size_t received = 0;
        switch (m_usb.Read(packet, timeoutMs, received)) {
            case UsbDevice::IoResult::Timeout: return std::nullopt;
            case UsbDevice::IoResult::Lost: lost = true; return std::nullopt;
            case UsbDevice::IoResult::Ok: break;
        }
        const std::span<const std::uint8_t> data(packet.data(), received);
        if (data.size() < 4) return std::nullopt;

        GipMessage message{data[0], data[1], data[2], {}};
        std::size_t index = 3;
        std::uint32_t length = 0;
        std::uint32_t offset = 0;
        if (!ReadVarint(data, index, length)) return std::nullopt;
        if ((message.options & kOptChunk) && !ReadVarint(data, index, offset)) return std::nullopt;
        const std::size_t available = std::min<std::size_t>(length, data.size() - std::min(index, data.size()));
        const auto body = data.subspan(index, available);

        if (!(message.options & kOptChunk)) {
            message.payload.assign(body.begin(), body.end());
            if ((message.options & kOptAck) && message.command != kCmdAck) {
                Acknowledge(message.command, message.options, message.sequence, length, 0);
            }
            return message;
        }

        Chunk& chunk = m_chunks[message.command];
        if (message.options & kOptChunkStart) {
            chunk = Chunk{offset, Bytes(body.begin(), body.end()), true};
        } else if (!chunk.active || body.empty()) {
            if (message.options & kOptAck) Acknowledge(message.command, message.options, message.sequence, offset, 0);
            return std::nullopt;
        } else {
            chunk.data.insert(chunk.data.end(), body.begin(), body.end());
        }
        const auto got = static_cast<std::uint32_t>(chunk.data.size());
        if (message.options & kOptAck) {
            Acknowledge(message.command, message.options, message.sequence, got, chunk.total > got ? chunk.total - got : 0);
        }
        if (got < chunk.total) return std::nullopt;
        chunk.active = false;
        message.payload = std::move(chunk.data);
        return message;
    }

private:
    struct Chunk {
        std::uint32_t total{0};
        Bytes data;
        bool active{false};
    };

    // Sends the GIP ACK of a received message with the bytes received so far and those still expected.
    void Acknowledge(std::uint8_t command, std::uint8_t options, std::uint8_t sequence, std::uint32_t got,
                     std::uint32_t remaining) {
        const std::array<std::uint8_t, 9> payload{
            0, command, static_cast<std::uint8_t>((options & 0x0F) | kOptInternal),
            static_cast<std::uint8_t>(got), static_cast<std::uint8_t>(got >> 8), 0, 0,
            static_cast<std::uint8_t>(remaining), static_cast<std::uint8_t>(remaining >> 8)};
        const Bytes packet = EncodePacket(kCmdAck, kOptInternal, sequence, payload);
        static_cast<void>(m_usb.Write(packet, kWriteTimeoutMs));
    }

    // Waits for the device's GIP ACK of a chunk; other messages are kept for the caller.
    bool WaitAck(std::uint8_t command, std::vector<GipMessage>& received) {
        const auto deadline = Clock::now() + kChunkAckWait;
        while (Clock::now() < deadline) {
            bool lost = false;
            auto message = Receive(50, lost);
            if (lost) return false;
            if (!message) continue;
            if (message->command == kCmdAck && message->payload.size() > 1 && message->payload[1] == command) return true;
            received.push_back(std::move(*message));
        }
        return false;
    }

    UsbDevice& m_usb;
    std::array<std::uint8_t, 256> m_sequences{};
    std::array<Chunk, 256> m_chunks{};
};

class SecurityHandshake final {
public:
    explicit SecurityHandshake(GipLink& link) : m_link(link) {}

    // Runs the GIP v1 handshake as PDP firmwares do: host hello, client hello, certificate, RSA-encrypted
    // premaster secret, host finish. The base never sends its client finish, it only ACKs the request
    // at GIP level and the host then sends control COMPLETE as Windows does; false with a reason on failure.
    bool Run(std::string& failure) {
        if (!Random(m_hostRandom)) {
            failure = "random generator unavailable";
            return false;
        }
        Bytes hello(m_hostRandom.begin(), m_hostRandom.end());
        hello.resize(kRandomSize + 8, 0);
        if (!SendData(kAuthHostHello, hello)) {
            failure = "could not send HOST_HELLO";
            return false;
        }

        const auto deadline = Clock::now() + kHandshakeDeadline;
        while (Clock::now() < deadline && !m_done && m_failure.empty()) {
            if (!m_pending.empty()) {
                GipMessage message = std::move(m_pending.front());
                m_pending.erase(m_pending.begin());
                Handle(message);
                continue;
            }
            bool lost = false;
            auto message = m_link.Receive(50, lost);
            if (lost) {
                m_failure = "base perdue pendant l'authentification";
                break;
            }
            if (message) Handle(*message);
        }
        if (!m_done && m_failure.empty()) m_failure = "timed out";
        failure = m_failure;
        return m_done;
    }

private:
    // Advances the handshake on one message; a GIP ACK of the CLIENT_FINISH request ends it.
    // Only control-context auth messages count, a non-zero error byte fails, and a record command that
    // differs from the header command means the base asks for the unsupported v2 handshake.
    void Handle(const GipMessage& message) {
        if (message.command == kCmdAck) {
            if (m_lastRequest == kAuthClientFinish && message.payload.size() > 1 && message.payload[1] == kCmdAuth) {
                Complete();
            }
            return;
        }
        if (message.command != kCmdAuth || message.payload.size() < 6) return;
        const auto& p = message.payload;
        if (p[0] != 0) return;
        if (p[2] != 0) {
            m_failure = std::format("the base answered error 0x{:02X}", p[2]);
            return;
        }
        if (p[1] & kAuthOptAck) {
            OnHandshakeAck();
            return;
        }
        if (p.size() < 10) return;
        if (p[3] != p[6]) {
            m_failure = "the base asks for handshake v2 (not supported)";
            return;
        }
        const std::span<const std::uint8_t> record(p.data() + 6, p.size() - 6);
        const std::span<const std::uint8_t> body(p.data() + 10, p.size() - 10);
        switch (p[6]) {
            case kAuthClientHello:
                if (body.size() < kRandomSize) {
                    m_failure = "CLIENT_HELLO trop court";
                    return;
                }
                std::copy_n(body.begin(), kRandomSize, m_clientRandom.begin());
                AppendTranscript(record);
                Request(kAuthCertificate, 1024);
                break;
            case kAuthCertificate:
                AppendTranscript(record);
                SendSecret(body);
                break;
            case kAuthClientFinish:
                Complete();
                break;
            default:
                break;
        }
    }

    // Answers the base's ACK of the last data record with the next step of the handshake.
    void OnHandshakeAck() {
        switch (m_lastSent) {
            case kAuthHostHello: Request(kAuthClientHello, kRandomSize + 48); break;
            case kAuthHostSecret: SendFinish(); break;
            case kAuthHostFinish: Request(kAuthClientFinish, 64); break;
            default: break;
        }
    }

    // Sends a random 48-byte premaster secret encrypted with the certificate's RSA key, then derives
    // the master secret from it and both randoms.
    void SendSecret(std::span<const std::uint8_t> certificate) {
        std::array<std::uint8_t, 48> premaster{};
        if (!Random(premaster)) {
            m_failure = "random generator unavailable";
            return;
        }
        auto encrypted = RsaPkcs1Encrypt(certificate, premaster);
        if (!encrypted) {
            m_failure = "no RSA key in the base's certificate";
            return;
        }
        m_master = Prf(premaster, "Master Secret", Randoms(), 48);
        if (!SendData(kAuthHostSecret, *encrypted)) m_failure = "could not send the host secret";
    }

    // Sends HOST_FINISH, the PRF of the master secret over the SHA-256 of the handshake transcript.
    void SendFinish() {
        const auto transcript = Sha256(m_transcript);
        const Bytes finish = Prf(m_master, "Host Finished", transcript, 32);
        if (!SendData(kAuthHostFinish, finish)) m_failure = "could not send HOST_FINISH";
    }

    // Sends the control COMPLETE message that closes the handshake and marks it done.
    void Complete() {
        static constexpr std::array<std::uint8_t, 2> kControlComplete{0x01, 0x00};
        if (m_link.Send(kCmdAuth, kOptInternal, kControlComplete)) {
            m_done = true;
        } else {
            m_failure = "could not send COMPLETE";
        }
    }

    // Sends a data message laid out as [context, options, error, command, length16], then the record
    // [command, version, length16, body], then an 8-byte zero trailer. The record joins the transcript.
    bool SendData(std::uint8_t command, std::span<const std::uint8_t> body) {
        Bytes record{command, 0x01};
        AppendBe16(record, body.size());
        record.insert(record.end(), body.begin(), body.end());
        Bytes message{0x00, kAuthOptAck | kAuthOptFromHost, 0x00, command};
        AppendBe16(message, record.size());
        message.insert(message.end(), record.begin(), record.end());
        message.resize(message.size() + kAuthTrailer, 0);
        AppendTranscript(record);
        m_lastSent = command;
        return SendMessage(message);
    }

    // Asks the base for one handshake record, announcing replySize plus 4 as the expected length.
    bool Request(std::uint8_t command, std::size_t replySize) {
        Bytes message{0x00, kAuthOptRequest | kAuthOptFromHost, 0x00, command};
        AppendBe16(message, replySize + 4);
        message.resize(message.size() + kAuthTrailer, 0);
        m_lastRequest = command;
        return SendMessage(message);
    }

    // Sends a handshake message in one packet when it fits, else in chunks; messages that arrive while
    // waiting for chunk ACKs are queued for Run.
    bool SendMessage(std::span<const std::uint8_t> message) {
        if (message.size() + 4 <= kPacketSize) return m_link.Send(kCmdAuth, kOptInternal | kOptAck, message);
        return m_link.SendChunked(kCmdAuth, message, m_pending);
    }

    void AppendTranscript(std::span<const std::uint8_t> record) {
        m_transcript.insert(m_transcript.end(), record.begin(), record.end());
    }

    // Returns the host random followed by the client random, the seed of the master secret.
    Bytes Randoms() const {
        Bytes seed(m_hostRandom.begin(), m_hostRandom.end());
        seed.insert(seed.end(), m_clientRandom.begin(), m_clientRandom.end());
        return seed;
    }

    GipLink& m_link;
    std::array<std::uint8_t, kRandomSize> m_hostRandom{};
    std::array<std::uint8_t, kRandomSize> m_clientRandom{};
    Bytes m_master;
    Bytes m_transcript;
    std::vector<GipMessage> m_pending;
    std::uint8_t m_lastSent{0};
    std::uint8_t m_lastRequest{0};
    bool m_done{false};
    std::string m_failure;
};

class XboxOneTransport final : public IPortalTransport {
public:
    // Opens the base, sends IDENTIFY (its descriptor reply is acknowledged by the link) and POWER ON,
    // then runs the security handshake, without which the base accepts nothing else.
    bool Open() override {
        if (!m_usb.Open(kVid, kPid)) {
            m_diagnostic = "Xbox One base found but it cannot be opened through WinUSB.";
            return false;
        }
        m_link.emplace(m_usb);
        Drain(300);

        static_cast<void>(m_link->Send(kCmdIdentify, kOptInternal, {}));
        Drain(1000);
        static constexpr std::array<std::uint8_t, 1> kPowerOn{0x00};
        static_cast<void>(m_link->Send(kCmdPower, kOptInternal, kPowerOn));
        Drain(100);

        std::string failure;
        SecurityHandshake handshake(*m_link);
        if (!handshake.Run(failure)) {
            m_diagnostic = "Xbox One base: authentication failed (" + failure +
                           "). Unplug the base and plug it back in.";
            crabe::shared::Logger::getInstance().info("Portal: Xbox One handshake failed: {}", failure);
            return false;
        }
        crabe::shared::Logger::getInstance().info("Portal: Xbox One base authenticated");
        return true;
    }

    void Close() override {
        m_link.reset();
        m_usb.Close();
    }

    // Sends the report as the payload of a non-system GIP 0x21 message.
    bool Write(const Report& report) override {
        return m_link && m_link->Send(kCmdDisney, 0x00, report);
    }

    // Waits for the next GIP 0x21 message and copies its payload into the report.
    ReadResult Read(Report& report, unsigned timeoutMs) override {
        if (!m_link) return ReadResult::Lost;
        const auto deadline = Clock::now() + std::chrono::milliseconds(timeoutMs);
        do {
            bool lost = false;
            auto message = m_link->Receive(timeoutMs, lost);
            if (lost) return ReadResult::Lost;
            if (message && message->command == kCmdDisney && !message->payload.empty()) {
                report.fill(0);
                std::copy_n(message->payload.begin(), std::min(message->payload.size(), kReportSize), report.begin());
                return ReadResult::Ok;
            }
        } while (Clock::now() < deadline);
        return ReadResult::Timeout;
    }

    PortalType Type() const override { return PortalType::XboxOne; }
    std::string Description() const override { return "Xbox One base (GIP / WinUSB)"; }
    std::string Diagnostic() const override { return m_diagnostic; }
    std::uint8_t RunningScanInterval() const override { return 3; }

private:
    // Reads and discards incoming messages for ms milliseconds, letting the link acknowledge them.
    void Drain(unsigned ms) {
        const auto deadline = Clock::now() + std::chrono::milliseconds(ms);
        while (Clock::now() < deadline) {
            bool lost = false;
            static_cast<void>(m_link->Receive(50, lost));
            if (lost) return;
        }
    }

    UsbDevice m_usb;
    std::optional<GipLink> m_link;
    std::string m_diagnostic;
};

} // namespace

std::unique_ptr<IPortalTransport> MakeXboxOneTransport() {
    return std::make_unique<XboxOneTransport>();
}

} // namespace crabe::infrastructure::portal
