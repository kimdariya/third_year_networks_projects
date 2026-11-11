#include "../include/WikipediaApi.hpp"
#include <cpr/cpr.h>
#include <algorithm>
#include <cctype>

static std::string wikiBase(const std::string& lang, const std::string& title) {
    return "https://" + lang + ".wikipedia.org/w/api.php?format=json&titles=" + cpr::util::urlEncode(title);
}

static std::string toLowerAscii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

static bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return false;
    return toLowerAscii(haystack).find(toLowerAscii(needle)) != std::string::npos;
}

static std::string stripHtml(std::string s) {
    std::string out;
    bool inTag = false;
    for (char c : s) {
        if (c == '<') { inTag = true; continue; }
        if (c == '>') { inTag = false; continue; }
        if (!inTag) out.push_back(c);
    }
    return out;
}

static bool nameMatches(const Json& item,
                        const std::string& englishTitle,
                        const std::string& romanised) {
    std::string title = item.value("title", "");
    std::string snippet = stripHtml(item.value("snippet", ""));
    std::string hay = title + " " + snippet;

    if (containsIgnoreCase(hay, englishTitle)) return true;
    if (!romanised.empty() && containsIgnoreCase(hay, romanised)) return true;

    auto tryWords = [&](const std::string& s) {
        std::string cur;
        for (unsigned char c : s) {
            if (std::isalnum(c)) {
                cur.push_back(static_cast<char>(c));
            } else {
                if (cur.size() >= 4 && containsIgnoreCase(hay, cur)) return true;
                cur.clear();
            }
        }
        return cur.size() >= 4 && containsIgnoreCase(hay, cur);
    };
    return tryWords(englishTitle) || tryWords(romanised);
}

fut::Future<std::string> WikipediaApi::searchRussianTitle(
    const std::string& englishTitle,
    const std::string& romanised,
    fut::CancellationToken token) const {

    std::string query = englishTitle + " Ghibli";
    std::string url = "https://ru.wikipedia.org/w/api.php?format=json&action=query&list=search&srsearch=" + cpr::util::urlEncode(query) + "&srlimit=10";

    return fut::then(http_.getJson(url, token), [englishTitle, romanised](Json j) {
        parse::checkWikiError(j);
        if (!j.contains("query") || !j["query"].contains("search")) {
            return std::string{};
        }
        const auto& results = j["query"]["search"];
        if (results.empty()) {
            return std::string{};
        }

        for (const auto& item : results) {
            std::string title = item.value("title", "");
            std::string lower = toLowerAscii(title);
            bool looksLikeFilm = lower.find("фильм") != std::string::npos ||
                                 lower.find("мультфильм") != std::string::npos ||
                                 lower.find("аниме") != std::string::npos;
            if (looksLikeFilm && nameMatches(item, englishTitle, romanised)) {
                return title;
            }
        }

        for (const auto& item : results) {
            if (nameMatches(item, englishTitle, romanised)) {
                return item.value("title", "");
            }
        }
        return std::string{};
    });
}

fut::Future<std::string> WikipediaApi::pageImage(
    const std::string& ruTitle,
    fut::CancellationToken token) const {

    std::string url = wikiBase("ru", ruTitle) + "&action=query&prop=pageimages&piprop=original|thumbnail&pilicense=any&redirects=1";
    return fut::then(http_.getJson(url, token), [](Json j) {
        return parse::pageImage(j);
    });
}

fut::Future<std::string> WikipediaApi::extract(
    const std::string& title,
    fut::CancellationToken token) const {

    std::string url = wikiBase("en", title) + "&action=query&prop=extracts&exintro=1&explaintext=1&redirects=1";
    return fut::then(http_.getJson(url, token), [](Json j) {
        return parse::extract(j);
    });
}