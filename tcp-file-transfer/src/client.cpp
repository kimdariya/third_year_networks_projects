#include "../include/client.h"
#include "../include/protocol.h"
#include "../include/socket.h"
#include "../include/speed_meter.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {
    bool rejected = false;
    constexpr std::size_t kChunk = 64 * 1024;
    constexpr int kIoTimeoutSec = 60;
}

Client::Client(std::string host, std::uint16_t port, std::string path)
    : host_(std::move(host)), port_(port), path_(std::move(path)) {}

bool Client::run() {
    std::error_code ec;
    std::uint64_t size = fs::file_size(path_, ec);
    if (ec) {
        throw std::system_error(ec, "file_size(" + path_ + ")");
    }

    std::ifstream in(path_, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open input file: " + path_);
    }

    FileHeader h;
    h.name = fs::path(path_).filename().u8string();
    h.size = size;

    Socket sock = make_connection(host_, port_);
    timeval tv{kIoTimeoutSec, 0};
    ::setsockopt(sock.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(sock.get(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    bool ok = false;
    bool ackStage = false;
    std::uint64_t sent = 0;
    try {
        protocol::send_header(sock, h);

        std::vector<char> buf(kChunk);
        while (sent < size) {
            std::size_t want = static_cast<std::size_t>(std::min<std::uint64_t>(kChunk, size - sent));
            in.read(buf.data(), static_cast<std::streamsize>(want));
            std::streamsize got = in.gcount();
            if (got <= 0) {
                throw std::runtime_error("short read from input file");
            }
            sock.send_all(buf.data(), static_cast<std::size_t>(got));
            sent += static_cast<std::uint64_t>(got);
        }
        ackStage = true;
        switch (protocol::recv_ack(sock)) {
            case protocol::Ack::Ok:
                ok = true;
                break;
            case protocol::Ack::Reject:
                std::fprintf(stderr, "[client] %s: REJECTED by server\n", path_.c_str());
                rejected = true;
                ok = true;
                break;
            case protocol::Ack::Fail:
                std::fprintf(stderr, "[client] %s: server reported failure\n", path_.c_str());
                break;
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[client] %s\n", e.what());
        if (!ackStage) {
            try {
                if (protocol::recv_ack(sock) == protocol::Ack::Fail)
                    std::fprintf(stderr, "[client] server reported failure\n");
            } catch (...) {}
        }
    }

    const char* status = rejected ? "REJECTED" : (ok ? "OK" : "FAILED");
    std::fprintf(stderr, "[client] %s: %s (%s)\n", path_.c_str(), status, human_size(sent).c_str());
    return ok;
}