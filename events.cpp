#include <cstdio>
#include <netinet/in.h>
#include <iostream>
#include "events.h"

namespace {
    // TODO remove
    void hexdump(char* buffer, size_t len){
        for (size_t i = 0; i < len; i++){
            printf("%02X ", *(buffer+i));
        }
    }
}
NewGame::NewGame(uint32_t maxx, uint32_t maxy, const std::vector<std::string> &players, uint32_t event_no)
        : maxx_(maxx), maxy_(maxy), players_(players) { event_no_ = event_no; };

NewGame::NewGame(char *buffer, size_t len, uint32_t event_no) {
    event_no_ = event_no;
    size_t ptr = 0;
    uint32_t x, y;
    memcpy(&x, buffer + ptr, 4);
    ptr += 4;
    memcpy(&y, buffer + ptr, 4);
    ptr += 4;
    size_t count = 0;
    maxx_ = ntohl(x);
    maxy_ = ntohl(y);
    while (ptr < len) {
        while (*(buffer + ptr + count) >= 33 && *(buffer + ptr + count) <= 126) {
            count += 1;
        }
        std::string player;
        player.assign(buffer + ptr, count);
        count++;
        ptr += count;
        count = 0;
        players_.push_back(player);
    }
}

Pixel::Pixel(char *buffer, size_t len, uint32_t event_no) {
    event_no_ = event_no;
    size_t ptr = 0;
    uint32_t x, y;
    memcpy(&playerNumber_, buffer, 1);
    ptr += 1;
    memcpy(&x, buffer + ptr, 4);
    x_ = ntohl(x);
    ptr += 4;
    memcpy(&y, buffer + ptr, 4);
    y_ = ntohl(y);
}

PlayerEliminated::PlayerEliminated(char *buffer, size_t len, uint32_t event_no) {
    event_no_ = event_no;
    memcpy(&playerNumber_, buffer, 1);
}

GameOver::GameOver() {};

size_t NewGame::toGuiBuffer(char* buffer) {
    size_t ptr = 0;
    sprintf(buffer, "NEW_GAME %u %u ", maxx_, maxy_);
    for (auto &p : players_) {
        ptr += sprintf(buffer + ptr, "%s\0", p.c_str());
    }
    ptr = strlen(buffer);
    sprintf(buffer + ptr, "\n");
    return strlen(buffer);
};

size_t Pixel::toGuiBuffer(char *buffer) {
    sprintf(buffer, "PIXEL %u %u %u\n", x_, y_, playerNumber_);
    return strlen(buffer);
}

size_t PlayerEliminated::toGuiBuffer(char *buffer) {
    sprintf(buffer, "PLAYER_ELIMINATED %u\n", playerNumber_);
    return strlen(buffer);
}

size_t GameOver::toGuiBuffer(char *buffer) {
    return 0; // nie bedziemy uzywac tej funkcji
}

size_t Event::toServerBuffer(char *buffer) {
    uint32_t event_no = htonl(event_no_);
    memcpy(buffer+4, &event_no, 4);
    uint8_t ev_type = eventType();
    memcpy(buffer+8, &ev_type, 1); //
    const size_t offset = 4;
    uint32_t len = 5;
    len += dataToBuffer(buffer+9);
    uint32_t len_hton = htonl(len);
    std::cout<<"dlugosc komunikatu "<<len<<"\n";
    memcpy(buffer, &len_hton, 4);
    std::cout<<"len_hton: "<<len_hton;
    hexdump(buffer, len+8);
    return len + 8; //TODO crc
}

size_t NewGame::dataToBuffer(char *buffer) {
    uint32_t len = 0;
    uint32_t maxx = htonl(maxx_);
    uint32_t maxy = htonl(maxy_);
    memcpy(buffer, &maxx, 4);
    memcpy(buffer+4, &maxy, 4);
    len = 8;
    for (auto &p : players_) {
        const char* pname = p.c_str();
        printf("Wysylane imie: %s\n", pname);
        memcpy(buffer+len, pname, p.length());
        //memcpy(buffer+len+1, "\0", 1);
        len += p.length() + 1;
    }
    printf("Wysylany bufer: %s\n", buffer);
    return len;
}

size_t Pixel::dataToBuffer(char *buffer) {
    memcpy(buffer, &playerNumber_, 1);
    uint32_t x = htonl(x_);
    uint32_t y = htonl(y_);
    memcpy(buffer + 1, &x, 4);
    memcpy(buffer + 5, &y, 4);
    return 9;
}

size_t PlayerEliminated::dataToBuffer(char *buffer) {
    memcpy(buffer, &playerNumber_, 1);
    return 1;
}

size_t GameOver::dataToBuffer(char *buffer) {
    return 0;
}