#pragma once
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include "Future.hpp"
#include "Models.hpp"
#include "HttpClient.hpp"

class GhibliApi {
public:
    explicit GhibliApi(HttpClient client)
        : http_(std::move(client)), cache_(std::make_shared<Cache>()) {}

    fut::Future<std::vector<Film>> allFilms() const;
    fut::Future<Film> filmById(const std::string& id) const;

private:
    struct Cache {
        std::mutex mutex;
        std::optional<std::vector<Film>> films;
    };

    HttpClient http_;
    std::shared_ptr<Cache> cache_;
};