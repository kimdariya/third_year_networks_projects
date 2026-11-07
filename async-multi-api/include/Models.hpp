#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

struct Film {
    std::string id, title, original_title, original_title_romanised;
    std::string description, director, producer;
    std::string release_date, running_time, rt_score;
};

struct FilmDetails {
    Film film;
    std::string imageUrl;
    std::string wikiExtract;
};

namespace parse {
    bool film(const Json& j, Film& out);
    std::vector<Film> films(const Json& j);
    std::string pageImage(const Json& j);
    std::string extract(const Json& j);
    void checkWikiError(const Json& j);
}