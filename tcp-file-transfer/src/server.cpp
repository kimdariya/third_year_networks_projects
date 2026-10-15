#include "../include/server.h"
#include "../include/protocol.h"
#include "../include/speed_meter.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <vector>

namespace {
    constexpr std::size_t kChunk = 64 * 1024;
    constexpr auto kReportPeriod = std::chrono::seconds(3);
    constexpr auto  kTick = std::chrono::milliseconds(100);
    constexpr int kRecvTimeoutSec = 60;
    constexpr std::size_t kMaxClients = 16;

    std::string peer_to_string(const sockaddr_in& a) {
        char buf[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &a.sin_addr, buf, sizeof(buf));
        return std::string(buf) + ":" + std::to_string(ntohs(a.sin_port));
    }
}

Server::Server(std::uint16_t port, std::string uploadsRoot)
    : port_(port),
      uploads_(std::move(uploadsRoot)),
      listener_(make_listener(port)) {
    reporter_ = std::thread([this]{ reporter_loop(); });
}

Server::~Server() {
    stop();
    listener_.close();
    if (reporter_.joinable()) reporter_.join();
    std::unique_lock<std::mutex> lk(mu_);
    idle_cv_.wait(lk, [this]{ return active_ == 0; });
}

void Server::stop() noexcept {
    stop_.store(true);
    int fd = listener_.get();
    if (fd >= 0) ::shutdown(fd, SHUT_RDWR);
}

void Server::run() {
    std::fprintf(stderr, "[server] listening on port %u, uploads -> %s\n", static_cast<unsigned>(port_), uploads_.root().c_str());

    while (!stop_.load()) {
        sockaddr_in peer{};
        socklen_t len = sizeof(peer);
        int cfd = ::accept(listener_.get(),reinterpret_cast<sockaddr*>(&peer), &len);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            if (stop_.load()) break;
            std::fprintf(stderr, "[server] accept: %s\n", std::strerror(errno));
            continue;
        }

        Socket sock(cfd);
        std::string peerStr = peer_to_string(peer);

        {
            std::lock_guard<std::mutex> lk(mu_);
            if (active_ >= kMaxClients) {
                std::fprintf(stderr, "[server] rejected %s: too many clients (%zu/%zu)\n",peerStr.c_str(), active_, kMaxClients);
                continue;
            }
            ++active_;
        }

        std::fprintf(stderr, "[server] accepted %s (fd=%d)\n", peerStr.c_str(), sock.get());

        try {
            std::thread([this, s = std::move(sock), peerStr]() mutable {
                try {
                    handle_client(std::move(s), peerStr);
                } catch (const std::exception& e) {
                    std::fprintf(stderr, "[server] client %s error: %s\n", peerStr.c_str(), e.what());
                }
                std::lock_guard<std::mutex> lk(mu_);
                if (--active_ == 0) idle_cv_.notify_all();
            }).detach();
        } catch (const std::exception& e) {
            std::fprintf(stderr, "[server] thread: %s\n", e.what());
            std::lock_guard<std::mutex> lk(mu_);
            --active_;
        }
    }
}

void Server::handle_client(Socket sock, std::string peer) {
    timeval tv{kRecvTimeoutSec, 0};
    ::setsockopt(sock.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    auto meter = std::make_shared<SpeedMeter>();
    {
        std::lock_guard<std::mutex> lk(mu_);
        clients_.emplace(sock.get(), ClientEntry{peer, meter, false});
    }

    bool ok = false;
    try {
        FileHeader h = protocol::recv_header(sock);

        std::fprintf(stderr, "[server] %s: start sending '%s' (%s)\n",
             peer.c_str(),
             h.name.c_str(),
             human_size(h.size).c_str());

        UploadFile out = uploads_.open_for_write(h.name);

        std::vector<std::uint8_t> buf(kChunk);
        for (std::uint64_t left = h.size; left > 0;) {
            auto n = static_cast<std::size_t>(std::min<std::uint64_t>(kChunk, left));
            sock.recv_all(buf.data(), n);
            out.write(buf.data(), n);
            meter->add(n);
            left -= n;
        }

        out.commit(h.size);
        ok = true;
        std::fprintf(stderr, "[server] %s: finished sending '%s' (%s) -> ok\n",
                     peer.c_str(),
                     h.name.c_str(),
                     human_size(h.size).c_str());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[server] %s: %s\n", peer.c_str(), e.what());
    }

    try { protocol::send_ack(sock, ok); } catch (...) {}

    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = clients_.find(sock.get());
        if (it != clients_.end()) {
            if (!it->second.reported) {
                std::fprintf(stderr, "[server][%s] final: instant=%s avg=%s\n",
                    peer.c_str(),
                    human_rate(meter->instant()).c_str(),
                    human_rate(static_cast<std::uint64_t>(meter->average())).c_str());
            }
            clients_.erase(it);
        }
    }
    ::shutdown(sock.get(), SHUT_RDWR);
}

void Server::reporter_loop() {
    auto next = std::chrono::steady_clock::now() + kReportPeriod;
    while (!stop_.load()) {
        std::this_thread::sleep_for(kTick);
        auto now = std::chrono::steady_clock::now();
        if (now < next) continue;
        next = now + kReportPeriod;

        std::lock_guard<std::mutex> lk(mu_);
        for (auto& entry : clients_) {
            std::fprintf(stderr, "[server][%s] instant=%s avg=%s total=%s\n",
                entry.second.peer.c_str(),
                human_rate(entry.second.meter->instant()).c_str(),
                human_rate(static_cast<std::uint64_t>(entry.second.meter->average())).c_str(),
                human_rate(entry.second.meter->total()).c_str());
            entry.second.reported = true;
        }
    }
}