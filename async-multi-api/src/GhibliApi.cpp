#include "../include/GhibliApi.hpp"
#include <stdexcept>

static constexpr const char* BASE = "https://ghibliapi.vercel.app";

fut::Future<std::vector<Film>> GhibliApi::allFilms() const {
    {
        std::lock_guard<std::mutex> lk(cache_->mutex);
        if (cache_->films) {
            return fut::ready(*cache_->films);
        }
    }
    return fut::then(http_.getJson(std::string(BASE) + "/films"), [cache = cache_](Json j) {
        auto films = parse::films(j);
        {
            std::lock_guard<std::mutex> lk(cache->mutex);
            cache->films = films;
        }
        return films;
    });
}

fut::Future<Film> GhibliApi::filmById(const std::string& id) const {
    if (id.empty()) {
        throw std::invalid_argument("film id is empty");
    }
    return fut::then(http_.getJson(std::string(BASE) + "/films/" + id), [](Json j) {
        Film f;
        if (!parse::film(j, f)) {
            throw std::runtime_error("invalid film JSON for id");
        }
        return f;
    });
}