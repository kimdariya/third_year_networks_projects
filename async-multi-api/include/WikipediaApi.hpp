#pragma once
#include <string>
#include "Future.hpp"
#include "Models.hpp"
#include "HttpClient.hpp"

class WikipediaApi {
public:
    explicit WikipediaApi(HttpClient client) : http_(std::move(client)) {}
    fut::Future<std::string> searchRussianTitle(const std::string& englishTitle) const;
    fut::Future<std::string> pageImage(const std::string& ruTitle) const;
    fut::Future<std::string> extract(const std::string& title) const;

private:
    HttpClient http_;
};