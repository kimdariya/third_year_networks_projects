#include "../include/server.h"

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>

namespace {

    int run_server(int argc, char** argv) {
        if (argc != 2) {
            std::fprintf(stderr, "usage: %s <port>\n", argv[0]);
            return 2;
        }
        int port = std::stoi(argv[1]);
        if (port < 1 || port > 65535)
            throw std::invalid_argument("port out of range (1..65535)");
        
        Server server(static_cast<std::uint16_t>(port), "uploads");
        server.run();
        return 0;
    }

}

int main(int argc, char** argv) {
    try {
        return run_server(argc, argv);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "server: %s\n", e.what());
        return 1;
    }
}