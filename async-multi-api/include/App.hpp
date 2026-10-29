#pragma once
#include "FilmService.hpp"

class App {
public:
    explicit App(FilmService service) : service_(std::move(service)) {}
    int run(int argc, char** argv) const;

private:
    FilmService service_;
};
