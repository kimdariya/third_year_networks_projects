#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <thread>

using namespace std::chrono;

constexpr uint16_t DEFAULT_PORT = 4446;
constexpr int SEND_INTERVAL_MS = 2000;
constexpr int SLEEP_STEP_MS = 100;
constexpr int TIMEOUT_MS = 6000;
constexpr int BUF_SIZE = 64;
constexpr const char* PEER_MSG = "PEER";
constexpr int PEER_MSG_LEN = 4;
constexpr int SELECT_TIMEOUT_US = 100000; //100 миллисекунд

extern std::atomic<bool> run;

struct Config {
    std::string group;
    uint16_t port;
    int family;
};

struct Sockets {
    int recv_sock = -1;
    int send_sock = -1;
};

bool parse_args(int argc, char** argv, Config& cfg);
