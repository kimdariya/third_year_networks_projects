#include "../include/FilmService.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>
#include <unicode/unistr.h>
#include <unicode/locid.h>

std::string toLowerUnicode(const std::string& utf8) {
    icu::UnicodeString u(utf8.c_str(), "UTF-8");
    std::string out;
    u.toLower(icu::Locale::getRoot()).toUTF8String(out);
    return out;
}

fut::Future<std::vector<Film>> FilmService::searchFilms(const std::string& query) const {
    fut::CancellationToken token;
    return fut::then(ghibli_.allFilms(token), [q = toLowerUnicode(query)](std::vector<Film> all) {
        std::vector<Film> out;
        for (auto& f : all) {
            if (toLowerUnicode(f.title).find(q) != std::string::npos ||
                toLowerUnicode(f.original_title_romanised).find(q) != std::string::npos) {
                out.push_back(std::move(f));
            }
        }
        return out;
    });
}

fut::Future<FilmDetails> FilmService::loadFilmDetails(const Film& selected) const {
    fut::CancellationToken token;

    auto imageFut = fut::then(
        wiki_.searchRussianTitle(selected.title, selected.original_title_romanised, token),
        [this, token](std::string ruTitle) {
            if (ruTitle.empty()) return fut::ready(std::string{}, token);
            return wiki_.pageImage(ruTitle, token);
        });

    auto extractFut = wiki_.extract(selected.title, token);

    auto safeImage = fut::recover(std::move(imageFut), [](const std::exception& e) {
        std::cerr << "[warn] image: " << e.what() << "\n";
        return std::string{};
    });

    auto safeExtract = fut::recover(std::move(extractFut), [](const std::exception& e) {
        std::cerr << "[warn] extract: " << e.what() << "\n";
        return std::string{};
    });

    return fut::then(std::move(safeImage), [selected, safeExtract](std::string img) {
        return fut::then(safeExtract, [selected, img](std::string ext) {
            FilmDetails d;
            d.film = selected;
            d.imageUrl = img;
            d.wikiExtract = ext;
            return d;
        });
    });
}