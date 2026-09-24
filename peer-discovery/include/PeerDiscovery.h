#pragma once
#include "common.h"

class PeerDiscovery {
public:
    explicit PeerDiscovery(const Config& cfg);
    ~PeerDiscovery();
    int running();

private:
    Config cfg_;
    Sockets socks_;
    std::map<std::string, steady_clock::time_point> peers_;
    std::thread sender_;
    std::thread receiver_;

    void setup_sockets();
    void sender_loop();
    void receiver_loop();
    void handle_packet(const char* buf, ssize_t n, const sockaddr_storage& from);
    void print_peers();
    bool check_timeouts();
};