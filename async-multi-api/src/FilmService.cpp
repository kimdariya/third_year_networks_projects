#include "../include/FilmService.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return s;
}

fut::Future<std::vector<Film>> FilmService::searchFilms(const std::string& query) const {
    return fut::then(ghibli_.allFilms(), [q = toLower(query)](std::vector<Film> all) {
        std::vector<Film> out;
        for (auto& f : all) {
            if (toLower(f.title).find(q) != std::string::npos ||
                toLower(f.original_title_romanised).find(q) != std::string::npos) {
                out.push_back(std::move(f));
            }
        }
        return out;
    });
}

fut::Future<FilmDetails> FilmService::loadFilmDetails(const Film& selected) const {
    auto imageFut = fut::then(wiki_.searchRussianTitle(selected.title),
        [this](std::string ruTitle) {
            if (ruTitle.empty()) return fut::ready(std::string{});
            return wiki_.pageImage(ruTitle);
        });

    auto cardFut = ghibli_.filmById(selected.id);
    auto extractFut = wiki_.extract(selected.title);

    auto safeImage = fut::recover(std::move(imageFut), [](const std::exception& e) {
        std::cerr << "[warn] image: " << e.what() << "\n";
        return std::string{};
    });

    auto safeExtract = fut::recover(std::move(extractFut), [](const std::exception& e) {
        std::cerr << "[warn] extract: " << e.what() << "\n";
        return std::string{};
    });


    return fut::then(std::move(cardFut), [safeImage, safeExtract](Film card) {
        return fut::then(safeImage, [card, safeExtract](std::string img) {
            return fut::then(safeExtract, [card, img](std::string ext) {
                FilmDetails d;
                d.film = card;
                d.imageUrl = img;
                d.wikiExtract = ext;
                return d;
            });
        });
    });
}