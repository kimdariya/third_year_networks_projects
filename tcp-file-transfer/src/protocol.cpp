#include "../include/protocol.h"

#include <stdexcept>

namespace {

    //границы для utf_8
    constexpr std::uint8_t kAsciiMax = 0x80;
    constexpr std::uint8_t kTwoByteLeadMask = 0xE0;
    constexpr std::uint8_t kTwoByteLeadBits = 0xC0;
    constexpr std::uint8_t kThreeByteLeadMask = 0xF0;
    constexpr std::uint8_t kThreeByteLeadBits = 0xE0;
    constexpr std::uint8_t kFourByteLeadMask = 0xF8;
    constexpr std::uint8_t kFourByteLeadBits = 0xF0;
    constexpr std::uint8_t kContinuationMask = 0xC0;
    constexpr std::uint8_t kContinuationBits = 0x80;

    // ack
    constexpr std::uint8_t kAckOk   = 0x01;
    constexpr std::uint8_t kAckFail = 0x00;

    constexpr int kU64Bytes = 8;

    void send_u64(const Socket& s, std::uint64_t v) {
        std::uint8_t b[kU64Bytes];
        for (int i = 0; i < kU64Bytes; ++i) {
            b[i] = static_cast<std::uint8_t>(v >> (56 - 8 * i));
        }
        s.send_all(b, sizeof(b));
    }

    std::uint64_t recv_u64(const Socket& s) {
        std::uint8_t b[kU64Bytes];
        s.recv_all(b, sizeof(b));
        std::uint64_t v = 0;
        for (int i = 0; i < kU64Bytes; ++i) {
            v = (v << 8) | b[i];
        }
        return v;
    }

}

namespace protocol {

    void validate(const FileHeader& h) {
        if (h.name.empty()) {
            throw std::invalid_argument("file name is empty");
        }
        if (h.name.size() > FileHeader::kMaxNameBytes) {
            throw std::invalid_argument("file name exceeds 4096 bytes");
        }
        if (h.name.find('\0') != std::string::npos) {
            throw std::invalid_argument("file name contains NUL");
        }
        if (h.size > FileHeader::kMaxFileSize) {
            throw std::invalid_argument("file size exceeds 1 TiB");
        }

        {
            const auto* p = reinterpret_cast<const unsigned char*>(h.name.data());
            const auto* end = p + h.name.size();
            while (p < end) {
                unsigned char c = *p;
                int extra;
                if (c < kAsciiMax) {
                    extra = 0;
                }else if ((c & kTwoByteLeadMask) == kTwoByteLeadBits){
                    extra = 1;
                }else if ((c & kThreeByteLeadMask) == kThreeByteLeadBits){
                    extra = 2;
                }else if ((c & kFourByteLeadMask) == kFourByteLeadBits){
                    extra = 3;
                }else {
                    throw std::invalid_argument("file name is not valid UTF-8");
                }

                if (p + extra >= end) {
                    throw std::invalid_argument("file name is not valid UTF-8");
                }

                for (int i = 1; i <= extra; ++i)
                    if ((p[i] & kContinuationMask) != kContinuationBits) {
                        throw std::invalid_argument("file name is not valid UTF-8");
                    }
                p += extra + 1;
            }
        }
    }

    void send_header(const Socket& s, const FileHeader& h) {
        validate(h);
        send_u64(s, h.name.size());
        s.send_all(h.name.data(), h.name.size());
        send_u64(s, h.size);
    }

    FileHeader recv_header(const Socket& s) {
        FileHeader h;

        std::uint64_t nameLen = recv_u64(s);
        if (nameLen == 0 || nameLen > FileHeader::kMaxNameBytes) {
            throw std::runtime_error("bad name length");
        }

        h.name.resize(static_cast<std::size_t>(nameLen));
        s.recv_all(h.name.data(), h.name.size());

        h.size = recv_u64(s);

        validate(h);
        return h;
    }

    void send_ack(const Socket& s, bool ok) {
        std::uint8_t b = ok ? kAckOk : kAckFail;
        s.send_all(&b, 1);
    }

    bool recv_ack(const Socket& s) {
        std::uint8_t b = 0;
        s.recv_all(&b, 1);
        return b == kAckOk;
    }

}