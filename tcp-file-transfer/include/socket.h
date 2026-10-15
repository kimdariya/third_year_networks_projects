#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class Socket {
public:
    Socket() noexcept = default;
    explicit Socket(int fd) noexcept;
    ~Socket();

    Socket(Socket&&) noexcept;//move-конструктор
    Socket& operator=(Socket&&) noexcept;//оператор перемещающего присваивания
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    int get() const noexcept;
    bool valid() const noexcept;
    void close() noexcept;

    void send_all(const void* data, std::size_t n) const;
    void recv_all(void* data, std::size_t n) const;

private:
    int fd_ = -1;
};


Socket make_listener(std::uint16_t port);
Socket make_connection(const std::string& host, std::uint16_t port);