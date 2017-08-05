#ifndef SIKTACKA_H
#define SIKTACKA_H

#include <cctype>
#include <cstdint>
#include <string>
#include <sys/time.h>
#include "err.h"
// #include "data_structures.h"

#define UI_SEND_SIZE 64
#define UI_TO_CLIENT_SIZE 30
#define CLIENT_TO_SERVER_SIZE 80
#define SERVER_TO_CLIENT_SIZE 512
#define MAX_PLAYERS 42

uint64_t get_timestamp();
uint16_t validate_port(const std::string &port);
#endif SIKTACKA_H