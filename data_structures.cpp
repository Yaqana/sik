#include <netinet/in.h>
#include <iostream>
#include "data_structures.h"


namespace {
    // TODO remove
    void hexdump(char* buffer, size_t len){
        printf("\n");
        for (size_t i = 0; i < len; i++){
            printf("%02X ", *(buffer+i));
        }
        printf("\n");
    }
}

size_t ClientData::toBuffer(char *buffer) {
    uint64_t s_id = htonll(session_id_);
    uint32_t event = htonl(next_event_);
    memcpy(buffer, &s_id, 8);
    memcpy(buffer + 8, &turn_direction_, 1); // nie trzeba zmieniac kolejnosci
    memcpy(buffer + 9, &event, 4);
    memcpy(buffer + 13, player_name_.c_str(), player_name_.size() + 1);
    return 14 + player_name_.size();
}

ClientData::ClientData(uint64_t session_id, int8_t turn_direction, uint32_t next_event,
                       const std::string &player_name):
    session_id_(session_id),
    turn_direction_(turn_direction),
    next_event_(next_event),
    player_name_(player_name) {};

size_t ServerData::toBuffer(char *buffer) const {
    size_t i = 0;
    uint32_t game_id = htonl(game_id_);
    memcpy(buffer, &game_id, 4);
    i = 4;
    for(auto &ev: events_) {
        char ev_buffer[EVENT_BUFFER_SIZE];
        size_t len = ev->ToServerBuffer(ev_buffer);
        memcpy(buffer + i, ev_buffer, len);
        i += len;
    }
    return i;
}

cdata_ptr buffer_to_client_data(char *buffer, size_t len) {
    uint64_t s_id;
    int8_t td;
    uint32_t event;
    memcpy(&s_id, buffer, 8);
    memcpy(&td, buffer + 8, 1);
    memcpy(&event, buffer + 9, 4);
    char* pname = (char *)malloc(len-12);
    memcpy(pname, buffer + 13, len - 13);
    cdata_ptr c_data = std::make_shared<ClientData>(
            ntohll(s_id),
            td,
            ntohl(event),
            std::string(pname)
    );
    free(pname);
    return c_data;
}


EventPtr buffer_to_event(char* buffer, size_t len) {
    uint32_t ptr = 0;
    uint32_t ev_no = 0;
    uint32_t event_type = 0;
    memcpy(&ev_no, buffer + ptr, 4);
    ptr += 4;
    memcpy(&event_type, buffer + ptr, 1);
    ptr += 1;
    switch (event_type) {
        case NEW_GAME:  {
            return std::make_shared<NewGame>(buffer+5, len-5, ntohl(ev_no));
        }
        case PIXEL: {
            return std::make_shared<Pixel>(buffer+5, len-5, ntohl(ev_no));
        }
        case PLAYER_ELIMINATED: {
            return std::make_shared<PlayerEliminated>(buffer+5, len-5, ntohl(ev_no));
        }
        case GAME_OVER: {
            return std::make_shared<GameOver>(ntohl(ev_no));
        }
        default: return nullptr;
    }
}

sdata_ptr buffer_to_server_data(char *buffer, size_t len) {
    uint16_t ptr = 0;
    uint32_t game_id;
    memcpy(&game_id, buffer, 4);
    ptr = 4;
    std::vector<EventPtr> events;
    while (ptr < len) {
        uint32_t event_len2;
        memcpy(&event_len2, buffer + ptr, 4);
        uint32_t event_len = ntohl(event_len2);
        ptr += 4;
        EventPtr e = buffer_to_event(buffer+ptr, event_len + 4);
        ptr += event_len + 4;
        events.push_back(e);
    }
    // TODO jeszcze crc
    return std::make_shared<ServerData>(ntohl(game_id), std::move(events));
}

