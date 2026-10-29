#include "../include/Models.hpp"

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

    Film film(const Json& j) {
        Film f;
        if (!j.is_object()) {
            return f;
        }
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
        return f;
    }

    std::vector<Film> films(const Json& j) {
        std::vector<Film> out;
        if (!j.is_array()) {
            return out;
        }
        for (const auto& item : j) {
            out.push_back(film(item));
        }
        return out;
    }

    std::string pageImage(const Json& j) {
        if (!j.is_object() || !j.contains("query") || !j["query"].contains("pages")) return {};
        for (auto& [id, page] : j["query"]["pages"].items()) {
            if (page.contains("original") && page["original"].contains("source") &&
                page["original"]["source"].is_string()) {
                return page["original"]["source"].get<std::string>();
            }
        }
        return {};
    }

    std::string extract(const Json& j) {
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