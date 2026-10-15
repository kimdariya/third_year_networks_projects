#include "../include/socket.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>

Socket::Socket(int fd) noexcept : fd_(fd) {}

Socket::~Socket() { close(); }

Socket::Socket(Socket&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }

Socket& Socket::operator=(Socket&& o) noexcept {
    if (this != &o) {
        close();
        fd_ = o.fd_;
        o.fd_ = -1;
    }
    return *this;
}

int  Socket::get()   const noexcept { return fd_; }
bool Socket::valid() const noexcept { return fd_ >= 0; }

void Socket::close() noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

void Socket::send_all(const void* data, std::size_t n) const {
    const auto* p = static_cast<const std::uint8_t*>(data);
    while (n > 0) {
        ssize_t w = ::send(fd_, p, n, MSG_NOSIGNAL);//С MSG_NOSIGNAL сервер не падает
        if (w < 0) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::generic_category(), "send");
        }
        if (w == 0) {
            throw std::runtime_error("send: peer closed");
        }
        p += w;
        n -= static_cast<std::size_t>(w);
    }
}

void Socket::recv_all(void* data, std::size_t n) const {
    auto* p = static_cast<std::uint8_t*>(data);
    while (n > 0) {
        ssize_t r = ::recv(fd_, p, n, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::generic_category(), "recv");
        }
        if (r == 0) {
            throw std::runtime_error("recv: peer closed");
        }
        p += r;
        n -= static_cast<std::size_t>(r);
    }
}


Socket make_listener(std::uint16_t port) {
    Socket s(::socket(AF_INET, SOCK_STREAM, 0));
    if (!s.valid()) {
        throw std::system_error(errno, std::generic_category(), "socket");
    }

    int one = 1;
    ::setsockopt(s.get(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(s.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::system_error(errno, std::generic_category(), "bind");
    }
    if (::listen(s.get(), SOMAXCONN) < 0) {
        throw std::system_error(errno, std::generic_category(), "listen");
    }
    return s;
}

Socket make_connection(const std::string& host, std::uint16_t port) {
    addrinfo hints{};//фильтруем только ipv4 tcp
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;
    std::string portStr = std::to_string(port);
    int rc = ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (rc != 0) {
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(rc));
    }

    Socket result;
    for (addrinfo* ai = res; ai != nullptr; ai = ai->ai_next) {
        Socket s(::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));
        if (!s.valid()) continue;
        if (::connect(s.get(), ai->ai_addr, ai->ai_addrlen) == 0) {
            result = std::move(s);
            break;
        }
    }
    ::freeaddrinfo(res);

    if (!result.valid()) {
        throw std::runtime_error("connect failed");
    }
    return result;
}