cmake_minimum_required(VERSION 3.29)
project(peer-discovery)

set(CMAKE_CXX_STANDARD 17)

add_executable(peer-discovery
        src/main.cpp
        src/parse_args.cpp
        src/PeerDiscovery.cpp)

target_include_directories(peer-discovery PRIVATE include)
