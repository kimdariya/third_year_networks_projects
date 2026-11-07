cmake_minimum_required(VERSION 3.16)
project(ghibli LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.10.5)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS_ALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(ghibli
        src/HttpClient.cpp
        src/GhibliApi.cpp
        src/WikipediaApi.cpp
        src/FilmService.cpp
        src/Models.cpp
        src/App.cpp
        src/main.cpp)

if(APPLE)
    find_program(BREW_EXECUTABLE brew
            PATHS /opt/homebrew/bin /usr/local/bin
            NO_DEFAULT_PATH)
    if(BREW_EXECUTABLE)
        execute_process(COMMAND ${BREW_EXECUTABLE} --prefix icu4c
                OUTPUT_VARIABLE ICU_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
        if(ICU_PREFIX)
            list(APPEND CMAKE_PREFIX_PATH "${ICU_PREFIX}")
        endif()
    endif()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)
target_include_directories(ghibli PRIVATE ${SODIUM_INCLUDE_DIRS})

find_library(SODIUM_LIBRARY NAMES sodium PATHS ${SODIUM_LIBRARY_DIRS} /opt/homebrew/lib /usr/local/lib)
if(NOT SODIUM_LIBRARY)
    message(FATAL_ERROR "libsodium library not found!")
endif()

target_link_libraries(ghibli PRIVATE ${SODIUM_LIBRARY})
find_package(ICU REQUIRED COMPONENTS uc)
target_link_libraries(ghibli PRIVATE cpr::cpr nlohmann_json::nlohmann_json ICU::uc)