#include "../include/common.h"

bool parse_args(int argc, char** argv, Config& cfg) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <group> [port] [iface]\n";
        return false;
    }

    cfg.group = argv[1];
    cfg.port = DEFAULT_PORT;

    if (argc > 2) {
        char* end = nullptr;
        long p = std::strtol(argv[2], &end, 10);
        if (end == argv[2] || *end == '\0' || p <= 0 || p > 65535) {
            std::cerr << "Bad port: " << argv[2] << "\n";
            return false;
        }
        cfg.port = static_cast<uint16_t>(p);
    }

    in_addr a4{};
    in6_addr a6{};
    if (inet_pton(AF_INET, cfg.group.c_str(), &a4) == 1) {
        uint8_t top = static_cast<uint8_t>(ntohl(a4.s_addr) >> 24);
        if (top < 224 || top > 239) {
            std::cerr << "Not a multicast IPv4 address\n";
            return false;
        }
        cfg.family = AF_INET;
    }else if (inet_pton(AF_INET6, cfg.group.c_str(), &a6) == 1) {
        if (a6.s6_addr[0] != 0xFF) {
            std::cerr << "Not a multicast IPv6 address\n";
            return false;
        }
        cfg.family = AF_INET6;
    }else {
        std::cerr << "Bad group\n";
        return false;
    }

    cfg.iface = (argc > 3) ? argv[3] : "";

    if (!cfg.iface.empty()) {
        if (cfg.family == AF_INET) {
            std::cerr << "Warning: interface is ignored for IPv4\n";
            cfg.iface.clear();
        } else if (if_nametoindex(cfg.iface.c_str()) == 0) {
            std::cerr << "Bad interface: " << cfg.iface << "\n";
            return false;
        }
    }

    return true;
}