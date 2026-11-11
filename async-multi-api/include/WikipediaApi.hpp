#pragma once
#include <string>
#include "Future.hpp"
#include "Models.hpp"
#include "HttpClient.hpp"

class WikipediaApi {
public:
    explicit WikipediaApi(HttpClient client) : http_(std::move(client)) {}
    fut::Future<std::string> searchRussianTitle(const std::string& englishTitle, const std::string& romanised = {}, fut::CancellationToken token = {}) const;
    fut::Future<std::string> pageImage(const std::string& ruTitle, fut::CancellationToken token = {}) const;
    fut::Future<std::string> extract(const std::string& title, fut::CancellationToken token = {}) const;

private:
    HttpClient http_;
};