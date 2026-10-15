#include "../include/uploads.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

UploadFile::UploadFile(Socket fd, fs::path path) : fd_(std::move(fd)), path_(std::move(path)) {}

UploadFile::~UploadFile() {
    if (done_) return;
    fd_.close();
    std::error_code ec;
    fs::remove(path_, ec);
}

void UploadFile::write(const void* data, std::size_t n) {
    const auto* p = static_cast<const std::uint8_t*>(data);
    while (n > 0) {
        ssize_t w = ::write(fd_.get(), p, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::generic_category(), "write");
        }
        p += w;
        n -= static_cast<std::size_t>(w);
    }
}

void UploadFile::commit(std::uint64_t expected) {
    struct stat st{};
    if (::fstat(fd_.get(), &st) != 0) {
        throw std::system_error(errno, std::generic_category(), "fstat");
    }
    if (static_cast<std::uint64_t>(st.st_size) != expected) {
        throw std::runtime_error("size mismatch");
    }
    if (::fsync(fd_.get()) != 0) {
        throw std::system_error(errno, std::generic_category(), "fsync");
    }
    done_ = true;
}


Uploads::Uploads(fs::path root) : root_(std::move(root)) {
    std::error_code ec;
    fs::create_directories(root_, ec);
    if (ec) throw std::system_error(ec, "create_directories(uploads)");

    root_ = fs::canonical(root_, ec);
    if (ec) throw std::system_error(ec, "canonical(uploads)");
}

const fs::path& Uploads::root() const noexcept { return root_; }

UploadFile Uploads::open_for_write(const std::string& client_name) const {
    if (client_name.find('\0') != std::string::npos)
        throw std::runtime_error("file name contains NUL");

    fs::path base = fs::u8path(client_name).filename();
    if (base.empty() || base == "." || base == "..")
        throw std::runtime_error("invalid file name");

    const std::string stem = base.stem().string();
    const std::string ext  = base.extension().string();

    for (int i = 0; i < 1000; ++i) {
        fs::path p = root_ / (i == 0 ? base.string() : stem + "_" + std::to_string(i) + ext);
        int fd = ::open(p.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
        if (fd >= 0) {
            return UploadFile(Socket(fd), std::move(p));
        }
        if (errno != EEXIST) {
            throw std::system_error(errno, std::generic_category(), "open(upload)");
        }
    }
    throw std::runtime_error("too many name collisions");
}