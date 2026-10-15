cmake_minimum_required(VERSION 3.16)
project(tcp-file-transfer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(ftcore STATIC
        src/socket.cpp
        src/protocol.cpp
        src/speed_meter.cpp
        src/uploads.cpp
        src/server.cpp
        src/client.cpp)
target_link_libraries(ftcore PUBLIC Threads::Threads)

add_executable(server src/server_main.cpp)
add_executable(client src/client_main.cpp)
target_link_libraries(server PRIVATE ftcore)
target_link_libraries(client PRIVATE ftcore)