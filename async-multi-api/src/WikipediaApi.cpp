#include "../include/WikipediaApi.hpp"
#include <cpr/cpr.h>
#include <algorithm>
#include <cctype>

static std::string wikiBase(const std::string& lang, const std::string& title) {
    return "https://" + lang + ".wikipedia.org/w/api.php?format=json&titles="
         + cpr::util::urlEncode(title);
}

static std::string toLowerAscii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

fut::Future<std::string> WikipediaApi::searchRussianTitle(const std::string& englishTitle) const {
    std::string url = "https://ru.wikipedia.org/w/api.php?format=json&action=query&list=search&srsearch="
                    + cpr::util::urlEncode(englishTitle) + "&srlimit=10";
    return fut::then(http_.getJson(url), [englishTitle](Json j) {
        parse::checkWikiError(j);
        if (!j.contains("query") || !j["query"].contains("search")) {
            return std::string{};
        }
        const auto& results = j["query"]["search"];
        if (results.empty()) {
            return std::string{};
        }

        const std::string needle = toLowerAscii(englishTitle);

        for (const auto& item : results) {
            std::string title = item.value("title", "");
            std::string lower = toLowerAscii(title);
            bool looksLikeFilm = lower.find("фильм") != std::string::npos ||
                                 lower.find("мультфильм") != std::string::npos ||
                                 lower.find("аниме") != std::string::npos;
            if (looksLikeFilm && (lower.find(needle) != std::string::npos ||
                                  needle.find(toLowerAscii(title.substr(0, std::min(title.size(), needle.size())))) != std::string::npos)) {
                return title;
            }
        }

        for (const auto& item : results) {
            std::string title = item.value("title", "");
            if (toLowerAscii(title).find(needle) != std::string::npos) {
                return title;
            }
        }

        for (const auto& item : results) {
            std::string title = item.value("title", "");
            std::string lower = toLowerAscii(title);
            if (lower.find("фильм") != std::string::npos ||
                lower.find("мультфильм") != std::string::npos) {
                return title;
            }
        }
        return std::string{};
    });
}

fut::Future<std::string> WikipediaApi::pageImage(const std::string& ruTitle) const {
    std::string url = wikiBase("ru", ruTitle) + "&action=query&prop=pageimages&piprop=original&pilicense=any&redirects=1";
    return fut::then(http_.getJson(url), [](Json j) {
        return parse::pageImage(j);
    });
}

fut::Future<std::string> WikipediaApi::extract(const std::string& title) const {
    std::string url = wikiBase("en", title) + "&action=query&prop=extracts&exintro=1&explaintext=1&redirects=1";
    return fut::then(http_.getJson(url), [](Json j) {
        return parse::extract(j);
    });
}