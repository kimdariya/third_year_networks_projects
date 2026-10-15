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
    void validate(const FileHeader&);
    void send_header(const Socket&, const FileHeader&);
    FileHeader recv_header(const Socket&);

    void send_ack(const Socket&, bool ok);
    bool recv_ack(const Socket&);
}