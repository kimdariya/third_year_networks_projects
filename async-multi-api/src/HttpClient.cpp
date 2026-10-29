#include "../include/HttpClient.hpp"
#include <cpr/cpr.h>
#include <stdexcept>

static constexpr int kTimeoutMs = 15000;

static cpr::Header headers() {
    return cpr::Header{{"User-Agent", "GhibliAsyncClient/1.0 (student project; your_email@example.com)"},
                       {"Accept", "application/json"}};
}

static std::string checkedBody(const cpr::Response& r, const std::string& url) {
    if (r.error.code != cpr::ErrorCode::OK)
        throw std::runtime_error("HTTP error: " + r.error.message);
    if (r.status_code < 200 || r.status_code >= 300)
        throw std::runtime_error("HTTP status " + std::to_string(r.status_code) + " for " + url);
    return r.text;
}

void HttpClient::keep(std::shared_ptr<void> pending) const {
    std::lock_guard<std::mutex> lk(keeper_->m);
    keeper_->items.push_back(std::move(pending));
}

fut::Future<std::string> HttpClient::get(const std::string& url) const {
    fut::Promise<std::string> p;
    auto out = p.future();
    auto pending = cpr::GetCallback(
        [p, url](cpr::Response r) {
            try {
                p.setValue(checkedBody(r, url));
            }catch (...) {
                p.setError(std::current_exception());
            }
            return 0;
        },
        cpr::Url{url}, headers(), cpr::Timeout{kTimeoutMs});
    keep(std::make_shared<decltype(pending)>(std::move(pending)));
    return out;
}

fut::Future<Json> HttpClient::getJson(const std::string& url) const {
    return fut::then(get(url), [](std::string body) {
        return Json::parse(body);
    });
}