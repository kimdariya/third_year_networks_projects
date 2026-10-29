#include "../include/WikipediaApi.hpp"
#include <cpr/cpr.h>

static std::string wikiBase(const std::string& lang, const std::string& title) {
    return "https://" + lang + ".wikipedia.org/w/api.php?format=json&titles="
         + cpr::util::urlEncode(title);
}

fut::Future<std::string> WikipediaApi::searchRussianTitle(const std::string& englishTitle) const {
    std::string url = "https://ru.wikipedia.org/w/api.php?format=json&action=query&list=search&srsearch="
                    + cpr::util::urlEncode(englishTitle) + "&srlimit=5";
    return fut::then(http_.getJson(url), [](Json j) {
        if (!j.contains("query") || !j["query"].contains("search")) {
            return std::string{};
        }
        for (const auto& item : j["query"]["search"]) {
            std::string title = item.value("title", "");
            if (title.find("фильм") != std::string::npos ||
                title.find("мультфильм") != std::string::npos) {
                return title;
            }
        }
        if (!j["query"]["search"].empty()) {
            return j["query"]["search"][0].value("title", "");
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