#include "../include/App.hpp"
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <sodium.h>

static std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return {};
    return s.substr(b, s.find_last_not_of(ws) - b + 1);
}

// printDetails без изменений
static void printDetails(const FilmDetails& d) {
    std::cout << "\n=== " << d.film.title
              << " (" << d.film.release_date << ") ===\n"
              << "Original : " << d.film.original_title
              << " / " << d.film.original_title_romanised << "\n"
              << "Director : " << d.film.director << "\n"
              << "Producer : " << d.film.producer << "\n"
              << "Runtime  : " << d.film.running_time << " min\n"
              << "RT score : " << d.film.rt_score << "%\n"
              << "Poster   : " << (d.imageUrl.empty() ? "(no image)" : d.imageUrl) << "\n\n"
              << "Description:\n" << d.film.description << "\n\n"
              << "Wikipedia extract:\n"
              << (d.wikiExtract.empty() ? "(no extract)" : d.wikiExtract) << "\n";
}

struct Filter {
    int yearFrom = -1;
    int yearTo = 9999;
    int minScore = -1;
};

static int toInt(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (...) {
        return -1;
    }
}

static bool passes(const Film& f, const Filter& fl) {
    int y = toInt(f.release_date);
    int s = toInt(f.rt_score);
    return y >= fl.yearFrom && y <= fl.yearTo && s >= fl.minScore;
}

static std::vector<Film> applyFilter(const std::vector<Film>& all, const Filter& fl) {
    std::vector<Film> out;
    for (const auto& f : all) {
        if (passes(f, fl)) out.push_back(f);
    }
    return out;
}

static void printHelp() {
    std::cout << "Commands:\n"
              << "  <number>      open film card\n"
              << "  ?             open random film from the list\n"
              << "  y <from> <to> filter by release year (e.g. y 1990 2005)\n"
              << "  r <min>       filter by min RT score (e.g. r 90)\n"
              << "  c             clear filters\n"
              << "  s             new search\n"
              << "  q             quit\n";
}

int App::run(int argc, char** argv) const {
    if (argc != 1) {
        std::cerr << "Usage: " << (argc > 0 && argv[0] ? argv[0] : "ghibli")
                  << "\n(no arguments expected; the query is entered interactively)\n";
        return 1;
    }

    for (;;) {
        std::string query;
        std::cout << "\nSearch Ghibli film (:q - quit): ";
        if (!std::getline(std::cin, query)) return 0;
        query = trim(query);
        if (query == ":q") return 0;
        if (query.empty()) {
            std::cout << "Empty query.\n";
            continue;
        }

        std::vector<Film> found;
        try {
            found = service_.searchFilms(query).get();
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
            continue;
        }
        if (found.empty()) {
            std::cout << "No films found.\n";
            continue;
        }

        Filter fl;

        for (;;) {
            auto shown = applyFilter(found, fl);

            std::cout << "\nMatches:\n";
            if (shown.empty()) std::cout << "  (nothing matches the filter)\n";
            for (size_t i = 0; i < shown.size(); ++i) {
                std::cout << "  [" << i << "] " << shown[i].title
                          << " (" << shown[i].original_title_romanised << ") "
                          << shown[i].release_date << ", RT " << shown[i].rt_score << "%\n";
            }
            printHelp();
            std::cout << "> ";

            std::string line;
            if (!std::getline(std::cin, line)) return 0;
            std::istringstream ss(trim(line));
            std::string cmd;
            ss >> cmd;

            if (cmd == "q") return 0;
            if (cmd == "s") break;
            if (cmd == "c") { fl = Filter{}; continue; }
            if (cmd == "y") {
                int a, b;
                if (ss >> a >> b) { fl.yearFrom = a; fl.yearTo = b; }
                else std::cout << "Usage: y <from> <to>\n";
                continue;
            }
            if (cmd == "r") {
                int m;
                if (ss >> m) fl.minScore = m;
                else std::cout << "Usage: r <min>\n";
                continue;
            }

            int idx;
            if (cmd == "?") {
                if (shown.empty()) { std::cout << "Nothing to pick.\n"; continue; }
                idx = static_cast<int>(randombytes_uniform(static_cast<uint32_t>(shown.size())));
            } else {
                idx = toInt(cmd);
            }

            if (idx < 0 || static_cast<size_t>(idx) >= shown.size()) {
                std::cout << "Invalid command or index.\n";
                continue;
            }

            try {
                FilmDetails details = service_.loadFilmDetails(shown[idx]).get();
                printDetails(details);
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << "\n";
            }
        }
    }
}