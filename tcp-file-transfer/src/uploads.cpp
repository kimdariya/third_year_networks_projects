#include "../include/uploads.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

namespace {
    constexpr int kNAME_MAX = 255;
    constexpr std::uint8_t kContinuationMask = 0xC0;// 1100 0000
    constexpr std::uint8_t kContinuationBits = 0x80;// 1000 0000

    std::string fit_name(std::string stem, std::string ext, const std::string& suffix, std::size_t maxName) {
        if (ext.size() + suffix.size() >= maxName) {
            ext.clear();
        }
        std::size_t budget = maxName - ext.size() - suffix.size();
        if (stem.size() > budget) {
            while (budget > 0 && (static_cast<unsigned char>(stem[budget]) & kContinuationMask) == kContinuationBits) {
                --budget;
            }
            stem.resize(budget);
        }
        return stem + suffix + ext;
    }
}

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

    long m = ::pathconf(root_.c_str(), _PC_NAME_MAX);
    const std::size_t maxName = m > 0 ? static_cast<std::size_t>(m) : kNAME_MAX;

    for (int i = 0; i < 1000; ++i) {
        std::string suffix = (i == 0) ? "" : "_" + std::to_string(i);
        fs::path p = root_ / fit_name(stem, ext, suffix, maxName);
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