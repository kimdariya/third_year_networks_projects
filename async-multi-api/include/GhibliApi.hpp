#pragma once
#include <string>
#include <vector>
#include "Future.hpp"
#include "Models.hpp"
#include "HttpClient.hpp"

class GhibliApi {
public:
    explicit GhibliApi(HttpClient client) : http_(std::move(client)) {}
    fut::Future<std::vector<Film>> allFilms() const;
    fut::Future<Film> filmById(const std::string& id) const;

private:
    HttpClient http_;
};