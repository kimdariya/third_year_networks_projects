#pragma once

#include "socket.h"

#include <cstdint>
#include <string>

struct FileHeader {
    std::string   name;
    std::uint64_t size = 0;

    static constexpr std::uint64_t kMaxNameBytes = 4096;
    static constexpr std::uint64_t kMaxFileSize  = 1ULL << 40;
};

namespace protocol {
    enum class Ack : std::uint8_t { Fail = 0x00, Ok = 0x01, Reject = 0x02 };

    void validate(const FileHeader&);
    void send_header(const Socket&, const FileHeader&);
    FileHeader recv_header(const Socket&);

    void send_ack(const Socket&, Ack);
    Ack  recv_ack(const Socket&);
}