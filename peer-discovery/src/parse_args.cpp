#include "../include/common.h"

//работает до создания объектов, поэтому нормалдаки без класса
bool parse_args(int argc, char** argv, Config& cfg) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <group> [port]\n";
        return false;
    }

    cfg.group = argv[1];
    cfg.port = DEFAULT_PORT;

    if (argc > 2) {
        char* end = nullptr;
        long p = std::strtol(argv[2], &end, 10);
        if (end != argv[2] && *end == '\0' && p > 0 && p <= 65535)
            cfg.port = static_cast<uint16_t>(p);
    }

    in_addr a4{};
    in6_addr a6{};
    if (inet_pton(AF_INET, cfg.group.c_str(), &a4) == 1)
        cfg.family = AF_INET;
    else if (inet_pton(AF_INET6, cfg.group.c_str(), &a6) == 1)
        cfg.family = AF_INET6;
    else {
        std::cerr << "Bad group\n";
        return false;
    }
    return true;
}