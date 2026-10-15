#pragma once

#include <cstdint>
#include <string>

class Client {
public:
    Client(std::string host, std::uint16_t port, std::string path);

    bool run();

private:
    std::string host_;
    std::uint16_t port_;
    std::string path_;
};