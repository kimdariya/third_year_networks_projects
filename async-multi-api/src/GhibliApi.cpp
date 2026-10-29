#include "../include/GhibliApi.hpp"
#include <stdexcept>

static constexpr const char* BASE = "https://ghibliapi.vercel.app";

fut::Future<std::vector<Film>> GhibliApi::allFilms() const {
    return fut::then(http_.getJson(std::string(BASE) + "/films"), [](Json j) {
        return parse::films(j);
    });
}

fut::Future<Film> GhibliApi::filmById(const std::string& id) const {
    if (id.empty()) {
        throw std::invalid_argument("film id is empty");
    }
    return fut::then(http_.getJson(std::string(BASE) + "/films/" + id), [](Json j) {
        return parse::film(j);
    });
}