#ifndef SIKTACKA_H
#define SIKTACKA_H

#include <cctype>
#include <cstdint>
#include <string>

#include "err.h"

#define CLIENT_TO_UI_SIZE 64
#define UI_TO_CLIENT_SIZE 30
#define CLIENT_TO_SERVER_SIZE 80
#define SERVER_TO_CLIENT_SIZE 512
#define EVENT_BUFFER_SIZE 512
#define MAX_PLAYERS 42

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

uint32_t GetCrc(char *buffer, size_t len);

int64_t GetTimestamp();

uint16_t ValidatePort(const std::string &port);


#endif // SIKTACKA_H