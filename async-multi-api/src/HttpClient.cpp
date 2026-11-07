#include "../include/HttpClient.hpp"
#include <algorithm>
#include <cpr/cpr.h>
#include <stdexcept>

static constexpr int kTimeoutMs = 15000;

static cpr::Header headers() {
    return cpr::Header{
        {"User-Agent", "GhibliAsyncClient/1.0 (https://github.com/kimdariya/third_year_networks_projects/tree/master/async-multi-api)"},
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

void HttpClient::unkeep(const void* raw) const {
    std::lock_guard<std::mutex> lk(keeper_->m);
    auto& items = keeper_->items;
    items.erase(std::remove_if(items.begin(), items.end(),
                               [raw](const std::shared_ptr<void>& p) { return p.get() == raw; }),
                items.end());
}

fut::Future<std::string> HttpClient::get(const std::string& url) const {
    return get(url, fut::CancellationToken{});
}

fut::Future<std::string> HttpClient::get(const std::string& url, fut::CancellationToken token) const {
    fut::Promise<std::string> p(token);
    auto out = p.future();

    auto slot = std::make_shared<std::shared_ptr<void>>();
    keep(slot);

    auto progress = cpr::ProgressCallback(
        [token](auto, auto, auto, auto, auto) -> bool {
            return !token.isCancelled();
        });

    auto pending = cpr::GetCallback(
        [p, url, token, this, slot](cpr::Response r) {
            unkeep(slot.get());
            if (token.isCancelled()) {
                p.setError(std::make_exception_ptr(std::runtime_error("operation cancelled")));
                return 0;
            }
            try {
                p.setValue(checkedBody(r, url));
            } catch (...) {
                p.setError(std::current_exception());
            }
            return 0;
        },
        cpr::Url{url},
        headers(),
        cpr::Timeout{kTimeoutMs},
        progress);

    *slot = std::make_shared<decltype(pending)>(std::move(pending));
    return out;
}

fut::Future<Json> HttpClient::getJson(const std::string& url) const {
    return getJson(url, fut::CancellationToken{});
}

fut::Future<Json> HttpClient::getJson(const std::string& url, fut::CancellationToken token) const {
    return fut::then(get(url, token), [](std::string body) {
        return Json::parse(body);
    });
}