#include "../include/client.h"

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>

namespace {

    int run_client(int argc, char** argv) {
        if (argc != 4) {
            std::fprintf(stderr, "usage: %s <file> <host> <port>\n", argv[0]);
            return 2;
        }
        int port = std::stoi(argv[3]);
        if (port < 1 || port > 65535)
            throw std::invalid_argument("port out of range (1..65535)");

        Client client(argv[2], static_cast<std::uint16_t>(port), argv[1]);
        return client.run() ? 0 : 1;
    }

}

int main(int argc, char** argv) {
    try {
        return run_client(argc, argv);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "client: %s\n", e.what());
        return 1;
    }
}