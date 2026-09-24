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
    uint16_t self_port_ = 0;
    std::atomic<bool> run_{true};
    
    static PeerDiscovery* instance_;

    static void on_sig(int);
    void setup_sockets();
    static void close_socks(Sockets& socks);
    void sender_loop();
    void receiver_loop();
    void handle_packet(const char* buf, ssize_t n, const sockaddr_storage& from);
    void print_peers();
    bool check_timeouts();
};