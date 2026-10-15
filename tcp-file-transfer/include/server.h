#pragma once

#include "socket.h"
#include "speed_meter.h"
#include "uploads.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

class Server {
public:
    Server(std::uint16_t port, std::string uploadsRoot);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run();
    void stop() noexcept;

private:
    void handle_client(Socket sock, std::string peer);
    void reporter_loop();

    std::uint16_t port_;
    Uploads uploads_;
    std::atomic<bool> stop_{false};
    Socket listener_;

    std::thread reporter_;

    struct ClientEntry {
        std::string peer;
        std::shared_ptr<SpeedMeter> meter;
        bool reported = false;
    };

    std::mutex mu_;
    std::unordered_map<int, ClientEntry> clients_;

    std::condition_variable idle_cv_;
    std::size_t active_ = 0;
};