#pragma once
#include <string>
#include <vector>
#include "Future.hpp"
#include "Models.hpp"
#include "GhibliApi.hpp"
#include "WikipediaApi.hpp"

class FilmService {
public:
    FilmService(GhibliApi ghibli, WikipediaApi wiki) : ghibli_(std::move(ghibli)), wiki_(std::move(wiki)) {}

    fut::Future<std::vector<Film>> searchFilms(const std::string& query) const;
    fut::Future<FilmDetails> loadFilmDetails(const Film& selected) const;

private:
    GhibliApi ghibli_;
    WikipediaApi wiki_;
};