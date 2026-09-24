#include "../include/PeerDiscovery.h"

std::atomic<bool> run{true};

static void on_sig(int) { run = false; }

PeerDiscovery::PeerDiscovery(const Config& cfg) : cfg_(cfg) {}

PeerDiscovery::~PeerDiscovery() {
    run = false;
    if (sender_.joinable()) {
        sender_.join();
    }

    if (receiver_.joinable()) {
        receiver_.join();
    }

    if (socks_.recv_sock >= 0) {
        close(socks_.recv_sock);
    }

    if (socks_.send_sock >= 0) {
        close(socks_.send_sock);
    }
}

int PeerDiscovery::running() {
    std::signal(SIGINT,  on_sig);
    std::signal(SIGTERM, on_sig);

    setup_sockets();
    if (socks_.recv_sock < 0 || socks_.send_sock < 0)
        return 1;

    sender_ = std::thread(&PeerDiscovery::sender_loop,   this);
    receiver_ = std::thread(&PeerDiscovery::receiver_loop, this);

    sender_.join();
    receiver_.join();
    return 0;
}

void PeerDiscovery::setup_sockets() {
    socks_.recv_sock = socket(cfg_.family, SOCK_DGRAM, 0);
    socks_.send_sock = socket(cfg_.family, SOCK_DGRAM, 0);
    if (socks_.recv_sock < 0 || socks_.send_sock < 0) {
        perror("socket");
        socks_.recv_sock = socks_.send_sock = -1;
        return;
    }

    int one = 1;
    setsockopt(socks_.recv_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(socks_.recv_sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));

    if (cfg_.family == AF_INET) {
        sockaddr_in ra{};
        ra.sin_family      = AF_INET;
        ra.sin_addr.s_addr = INADDR_ANY;
        ra.sin_port        = htons(cfg_.port);
        if (bind(socks_.recv_sock, reinterpret_cast<sockaddr*>(&ra), sizeof(ra)) < 0) {
            perror("bind recv");
            socks_.recv_sock = socks_.send_sock = -1;
            return;
        }

        ip_mreq m{};
        inet_pton(AF_INET, cfg_.group.c_str(), &m.imr_multiaddr);
        m.imr_interface.s_addr = INADDR_ANY;
        if (setsockopt(socks_.recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m, sizeof(m)) < 0) {
            perror("join4");
            socks_.recv_sock = socks_.send_sock = -1;
            return;
        }

        sockaddr_in sa{};
        sa.sin_family      = AF_INET;
        sa.sin_addr.s_addr = INADDR_ANY;
        sa.sin_port        = 0;
        if (bind(socks_.send_sock, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0) {
            perror("bind send");
            socks_.recv_sock = socks_.send_sock = -1;
            return;
        }

        unsigned char ttl   = 1;
        unsigned char sloop = 1;
        setsockopt(socks_.send_sock, IPPROTO_IP, IP_MULTICAST_TTL,  &ttl,   sizeof(ttl));
        setsockopt(socks_.send_sock, IPPROTO_IP, IP_MULTICAST_LOOP, &sloop, sizeof(sloop));

    } else {
        sockaddr_in6 ra{};
        ra.sin6_family = AF_INET6;
        ra.sin6_addr   = in6addr_any;
        ra.sin6_port   = htons(cfg_.port);
        if (bind(socks_.recv_sock, reinterpret_cast<sockaddr*>(&ra), sizeof(ra)) < 0) {
            perror("bind recv");
            socks_.recv_sock = socks_.send_sock = -1;
            return;
        }

        ipv6_mreq m{};
        inet_pton(AF_INET6, cfg_.group.c_str(), &m.ipv6mr_multiaddr);
        m.ipv6mr_interface = 0;
        if (setsockopt(socks_.recv_sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &m, sizeof(m)) < 0) {
            perror("join6");
            socks_.recv_sock = socks_.send_sock = -1;
            return;
        }

        sockaddr_in6 sa{};
        sa.sin6_family = AF_INET6;
        sa.sin6_addr   = in6addr_any;
        sa.sin6_port   = 0;
        if (bind(socks_.send_sock, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0) {
            perror("bind send");
            socks_.recv_sock = socks_.send_sock = -1;
            return;
        }

        int hops = 1;
        unsigned int sloop6 = 1;
        setsockopt(socks_.send_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops,   sizeof(hops));
        setsockopt(socks_.send_sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &sloop6, sizeof(sloop6));
    }
}

void PeerDiscovery::sender_loop() {
    while (run) {
        if (cfg_.family == AF_INET) {
            sockaddr_in d{};
            d.sin_family = AF_INET;
            d.sin_port   = htons(cfg_.port);
            inet_pton(AF_INET, cfg_.group.c_str(), &d.sin_addr);
            sendto(socks_.send_sock, PEER_MSG, PEER_MSG_LEN, 0, reinterpret_cast<sockaddr*>(&d), sizeof(d));
        } else {
            sockaddr_in6 d{};
            d.sin6_family = AF_INET6;
            d.sin6_port   = htons(cfg_.port);
            inet_pton(AF_INET6, cfg_.group.c_str(), &d.sin6_addr);
            sendto(socks_.send_sock, PEER_MSG, PEER_MSG_LEN, 0, reinterpret_cast<sockaddr*>(&d), sizeof(d));
        }

        for (int i = 0; i < SEND_INTERVAL_MS / SLEEP_STEP_MS && run; ++i)
            std::this_thread::sleep_for(milliseconds(SLEEP_STEP_MS));
    }
}

void PeerDiscovery::receiver_loop() {
    char buf[BUF_SIZE];

    while (run) {
        fd_set rf;
        FD_ZERO(&rf);
        FD_SET(socks_.recv_sock, &rf);
        timeval tv{0, SELECT_TIMEOUT_US};

        int res = select(socks_.recv_sock + 1, &rf, nullptr, nullptr, &tv);
        if (res > 0 && FD_ISSET(socks_.recv_sock, &rf)) {
            sockaddr_storage from{};
            socklen_t fl = sizeof(from);
            ssize_t n = recvfrom(socks_.recv_sock, buf, sizeof(buf) - 1, 0, reinterpret_cast<sockaddr*>(&from), &fl);
            if (n > 0) {
                buf[n] = '\0';
                handle_packet(buf, n, from);
            }
        }
        if (check_timeouts())
            print_peers();
    }
}

void PeerDiscovery::handle_packet(const char* buf, ssize_t n, const sockaddr_storage& from) {
    if (n < PEER_MSG_LEN) return;
    if (std::strncmp(buf, PEER_MSG, PEER_MSG_LEN) != 0) return;

    char ip[INET6_ADDRSTRLEN]{};
    uint16_t port = 0;

    if (from.ss_family == AF_INET) {
        const auto* a = reinterpret_cast<const sockaddr_in*>(&from);
        inet_ntop(AF_INET, &a->sin_addr, ip, sizeof(ip));
        port = ntohs(a->sin_port);
    } else if (from.ss_family == AF_INET6) {
        const auto* a = reinterpret_cast<const sockaddr_in6*>(&from);
        inet_ntop(AF_INET6, &a->sin6_addr, ip, sizeof(ip));
        port = ntohs(a->sin6_port);
    } else {
        return;
    }

    std::string key = std::string(ip) + ":" + std::to_string(port);
    bool is_new = (peers_.find(key) == peers_.end());
    peers_[key] = steady_clock::now();

    if (is_new) {
        std::cout << "[+] " << key << "\n";
        print_peers();
    }
}

void PeerDiscovery::print_peers() {
    std::cout << "Alive peers (" << peers_.size() << "):\n";
    for (const auto& p : peers_)
        std::cout << "  " << p.first << "\n";
    std::cout << std::flush;
}

bool PeerDiscovery::check_timeouts() {
    bool changed = false;
    auto now = steady_clock::now();
    for (auto it = peers_.begin(); it != peers_.end();) {
        if (duration_cast<milliseconds>(now - it->second).count() > TIMEOUT_MS) {
            std::cout << "[-] " << it->first << "\n";
            it = peers_.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    return changed;
}