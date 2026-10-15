#include "../include/server.h"

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>

namespace {

    int run_server(int argc, char** argv) {
        if (argc < 2 || argc > 3) {
            std::fprintf(stderr, "usage: %s <port> [uploads-dir]\n", argv[0]);
            std::fprintf(stderr, "uploads-dir  where to save received files (default: uploads)\n");
            return 2;
        }
        int port = std::stoi(argv[1]);
        if (port < 1 || port > 65535)
            throw std::invalid_argument("port out of range (1..65535)");

        std::string uploadsDir = (argc == 3) ? argv[2] : "uploads";
        Server server(static_cast<std::uint16_t>(port), std::move(uploadsDir));
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