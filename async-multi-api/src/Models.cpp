#include "../include/Models.hpp"
#include <stdexcept>

static std::string str(const Json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) {
        return {};
    }
    if (it->is_string()) {
        return it->get<std::string>();
    }
    return it->dump();
}

namespace parse {

    void checkWikiError(const Json& j) {
        if (j.is_object() && j.contains("error")) {
            const auto& err = j["error"];
            std::string code = err.value("code", "unknown");
            std::string info = err.value("info", "Wikipedia API error");
            throw std::runtime_error("Wikipedia API error [" + code + "]: " + info);
        }

        if (j.is_object() && j.contains("errors") && j["errors"].is_array() && !j["errors"].empty()) {
            const auto& err = j["errors"][0];
            std::string code = err.value("code", "unknown");
            std::string info = err.value("info", err.value("text", "Wikipedia API error"));
            throw std::runtime_error("Wikipedia API error [" + code + "]: " + info);
        }
    }

    bool film(const Json& j, Film& out) {
        if (!j.is_object()) {
            return false;
        }

        Film f;
        f.id = str(j, "id");
        f.title = str(j, "title");
        f.original_title = str(j, "original_title");
        f.original_title_romanised = str(j, "original_title_romanised");
        f.description = str(j, "description");
        f.director = str(j, "director");
        f.producer = str(j, "producer");
        f.release_date = str(j, "release_date");
        f.running_time = str(j, "running_time");
        f.rt_score = str(j, "rt_score");

        if (f.id.empty() || f.title.empty() || f.release_date.empty() || f.rt_score.empty()) {
            return false;
        }
        out = std::move(f);
        return true;
    }

    std::vector<Film> films(const Json& j) {
        std::vector<Film> out;
        if (!j.is_array()) {
            return out;
        }
        for (const auto& item : j) {
            Film f;
            if (film(item, f)) {
                out.push_back(std::move(f));
            }
        }
        return out;
    }

    std::string pageImage(const Json& j) {
        checkWikiError(j);
        if (!j.is_object() || !j.contains("query") || !j["query"].contains("pages")) return {};
        for (auto& [id, page] : j["query"]["pages"].items()) {
            if (page.contains("original") && page["original"].contains("source") &&
                page["original"]["source"].is_string()) {
                return page["original"]["source"].get<std::string>();
            }
            if (page.contains("thumbnail") && page["thumbnail"].contains("source") &&
                page["thumbnail"]["source"].is_string()) {
                return page["thumbnail"]["source"].get<std::string>();
            }
        }
        return {};
    }

    std::string extract(const Json& j) {
        checkWikiError(j);
        if (!j.is_object() || !j.contains("query") || !j["query"].contains("pages")) {
            return {};
        }
        for (auto& [id, page] : j["query"]["pages"].items()) {
            if (page.contains("extract") && page["extract"].is_string()) {
                return page["extract"].get<std::string>();
            }
        }
        return {};
    }

}