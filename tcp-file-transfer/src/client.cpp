#include "../include/client.h"
#include "../include/protocol.h"
#include "../include/socket.h"
#include "../include/speed_meter.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {
    constexpr std::size_t kChunk = 64 * 1024;
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

    bool ok = false;
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
        ok = protocol::recv_ack(sock);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[client] %s\n", e.what());
    }

    std::fprintf(stderr, "[client] %s: %s (%s)\n",
             path_.c_str(), ok ? "OK" : "FAILED",
             human_size(sent).c_str());
    return ok;
}