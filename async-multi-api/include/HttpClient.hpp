#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "Future.hpp"
#include "Models.hpp"

class HttpClient {
public:
    fut::Future<std::string> get(const std::string& url) const;
    fut::Future<Json> getJson(const std::string& url) const;

private:
    struct Keeper {
        std::mutex m;
        std::vector<std::shared_ptr<void>> items;
    };
    void keep(std::shared_ptr<void> pending) const;
    std::shared_ptr<Keeper> keeper_ = std::make_shared<Keeper>();
};