#include <cstdio>
#include <netinet/in.h>
#include <iostream>
#include "events.h"
#include "siktacka.h"

namespace {
    // TODO remove
    void hexdump(char* buffer, size_t len){
        for (size_t i = 0; i < len; i++){
            printf("%02X ", *(buffer+i));
        }
    }
}

EventPtr Event::NewEvent(char *buffer, size_t len) {
    uint32_t ptr = 4; // skipping len field (already parsed)
    uint32_t ev_no = 0;
    uint32_t event_type = 0;
    memcpy(&ev_no, buffer + ptr, 4);
    ptr += 4;
    memcpy(&event_type, buffer + ptr, 1);
    ptr += 1;

    uint32_t expected_crc = getCrc(buffer, len - 4);
    uint32_t real_crc;
    memcpy(&real_crc, buffer + len - 4, 4);
    if (ntohl(real_crc) != expected_crc){
        return nullptr;
    }

    size_t event_len = len - ptr -4;
    switch (event_type) {
        case 0:  {
            return NewGame::New(buffer+ptr, event_len, ntohl(ev_no));
        }
        case 1: {
            return Pixel::New(buffer+ptr, event_len, ntohl(ev_no));
        }
        case 2: {
            return PlayerEliminated::New(buffer+ptr, event_len, ntohl(ev_no));
        }
        case 3: {
            return GameOver::New(ntohl(ev_no));
        }
        default: return nullptr;
    }
}

size_t Event::ToServerBuffer(char *buffer) const {
    uint32_t event_no = htonl(event_no_);
    uint8_t ev_type = EventType();

    memcpy(buffer+4, &event_no, 4);
    memcpy(buffer+8, &ev_type, 1);

    uint32_t len = 5; // length of event_ fields
    len += DataToBuffer(buffer + 9);
    uint32_t len_hton = htonl(len);
    memcpy(buffer, &len_hton, 4);

    uint32_t crc = getCrc(buffer, len + 4);
    uint32_t crc_hton = htonl(crc);

    memcpy(buffer + len + 4, &crc_hton, 4);

    return len + 8;
}


NewGame::NewGame(uint32_t maxx, uint32_t maxy, const std::vector<std::string> &players, uint32_t event_no)
        : maxx_(maxx), maxy_(maxy), players_(players) {
    event_no_ = event_no;
    len_ += 8;
    for (auto &p: players)
        len_ += p.size() + 1;
};

std::shared_ptr<NewGame> NewGame::New(char *buffer, size_t len, uint32_t event_no) {
    size_t ptr = 0;

    uint32_t x, y;
    memcpy(&x, buffer + ptr, 4);
    ptr += 4;
    memcpy(&y, buffer + ptr, 4);
    ptr += 4;

    size_t count = 0;
    std::vector<std::string> players;
    while (ptr < len) { // CRC
        while (*(buffer + ptr + count) >= 33 && *(buffer + ptr + count) <= 126) {
            count += 1;
        }
        std::string player;
        player.assign(buffer + ptr, count + 1);
        count++;
        ptr += count;
        count = 0;
        players.push_back(player);
    }

    uint32_t maxx = ntohl(x);
    uint32_t maxy = ntohl(y);
    return std::make_shared<NewGame>(maxx, maxy, std::move(players), event_no);
}

size_t NewGame::DataToBuffer(char *buffer) const {
    uint32_t len = 0;
    uint32_t maxx = htonl(maxx_);
    uint32_t maxy = htonl(maxy_);

    memcpy(buffer, &maxx, 4);
    memcpy(buffer+4, &maxy, 4);
    len = 8;

    for (auto &p : players_) {
        const char* pname = p.c_str();
        memcpy(buffer+len, pname, p.length() + 1);
        len += p.length() + 1;
    }

    return len;
}

size_t NewGame::ToGuiBuffer(char *buffer) const {
    size_t ptr = 0;
    sprintf(buffer, "NEW_GAME %u %u ", maxx_, maxy_);

    ptr += strlen(buffer);
    for (auto &p : players_) {
        ptr += sprintf(buffer + ptr, "%s ", p.c_str());
    }

    sprintf(buffer + ptr, "\n");
    return strlen(buffer);
};

Pixel::Pixel(uint32_t x, uint32_t y, uint8_t number, uint32_t event_no)
        : x_(x), y_(y), playerNumber_(number) {
    event_no_ = event_no;
    len_ += 9;
}

std::shared_ptr<Pixel> Pixel::New(char *buffer, size_t len, uint32_t event_no) {
    size_t ptr = 0;
    uint32_t x, y;
    uint8_t playerNumber;
    memcpy(&playerNumber, buffer, 1);
    ptr += 1;
    memcpy(&x, buffer + ptr, 4);
    ptr += 4;
    memcpy(&y, buffer + ptr, 4);
    return std::make_shared<Pixel>(ntohl(x), ntohl(y), playerNumber, event_no);
}

size_t Pixel::DataToBuffer(char *buffer) const {
    memcpy(buffer, &playerNumber_, 1);
    uint32_t x = htonl(x_);
    uint32_t y = htonl(y_);
    memcpy(buffer + 1, &x, 4);
    memcpy(buffer + 5, &y, 4);
    return 9;
}

size_t Pixel::ToGuiBuffer(char *buffer) const {
    sprintf(buffer, "PIXEL %u %u %s\n", x_, y_, player_name_.c_str());
    return strlen(buffer);
}

void Pixel::MapName(const std::vector<std::string> &players){
    if (playerNumber_ < players.size())
        player_name_ = players[playerNumber_];
}

PlayerEliminated::PlayerEliminated(uint8_t player_numer, uint32_t event_no)
        : playerNumber_(player_numer){
    event_no_ = event_no;
    len_ += 1;
}

std::shared_ptr<PlayerEliminated> PlayerEliminated::New(char *buffer, size_t len, uint32_t event_no) {
    uint8_t playerNumber;
    memcpy(&playerNumber, buffer, 1);
    return std::make_shared<PlayerEliminated>(playerNumber, event_no);
}

size_t PlayerEliminated::DataToBuffer(char *buffer) const {
    memcpy(buffer, &playerNumber_, 1);
    return 1;
}

size_t PlayerEliminated::ToGuiBuffer(char *buffer) const {
    sprintf(buffer, "PLAYER_ELIMINATED %s\n", player_name_.c_str());
    return strlen(buffer);
}

void PlayerEliminated::MapName(const std::vector<std::string> &players){
    if (playerNumber_ < players.size())
        player_name_ = players[playerNumber_];
}

std::shared_ptr<GameOver> GameOver::New(uint32_t event_no) {
    return std::make_shared<GameOver>(event_no);
}

size_t GameOver::ToGuiBuffer(char *buffer) const {
    return 0; // nie bedziemy uzywac tej funkcji
}

size_t GameOver::DataToBuffer(char *buffer) const {
    return 0;
}
