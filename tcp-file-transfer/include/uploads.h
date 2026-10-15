#pragma once

#include "socket.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

class UploadFile {
public:
    UploadFile(Socket fd, std::filesystem::path path);
    ~UploadFile();

    UploadFile(const UploadFile&) = delete;
    UploadFile& operator=(const UploadFile&) = delete;

    void write(const void* data, std::size_t n);
    void commit(std::uint64_t expectedSize);

private:
    Socket fd_;
    std::filesystem::path path_;
    bool done_ = false;
};

class Uploads {
public:
    explicit Uploads(std::filesystem::path root);

    UploadFile open_for_write(const std::string& client_name) const;

    const std::filesystem::path& root() const noexcept;

private:
    std::filesystem::path root_;
};