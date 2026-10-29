#include <iostream>
#include "../include/App.hpp"

int runApp(int argc, char** argv) {
    HttpClient http;
    FilmService service{GhibliApi(http), WikipediaApi(http)};
    return App(std::move(service)).run(argc, argv);
}

int main(int argc, char** argv) {
    try {
        return runApp(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}