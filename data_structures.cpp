#include <netinet/in.h>
#include <iostream>
#include "data_structures.h"

std::shared_ptr<ClientData> ClientData::New(char *buffer, size_t len) {
    uint64_t s_id;
    int8_t td;
    uint32_t event;
    memcpy(&s_id, buffer, 8);
    memcpy(&td, buffer + 8, 1);
    memcpy(&event, buffer + 9, 4);
    char *pname = (char *) malloc(len - 12);
    memcpy(pname, buffer + 13, len - 13);
    ClientDataPtr c_data = std::make_shared<ClientData>(
            ntohll(s_id),
            td,
            ntohl(event),
            std::string(pname)
    );
    free(pname);
    return c_data;
}

ClientData::ClientData(uint64_t session_id, int8_t turn_direction,
                       uint32_t next_event,
                       const std::string &player_name) :
        session_id_(session_id),
        turn_direction_(turn_direction),
        next_event_(next_event),
        player_name_(player_name),
        game_id_(0) {};

size_t ClientData::ToBuffer(char *buffer) {
    uint64_t s_id = htonll(session_id_);
    uint32_t event = htonl(next_event_);
    memcpy(buffer, &s_id, 8);
    memcpy(buffer + 8, &turn_direction_, 1);
    memcpy(buffer + 9, &event, 4);
    memcpy(buffer + 13, player_name_.c_str(), player_name_.size() + 1);
    return 14 + player_name_.size();
}

size_t ServerData::ToBuffer(char *buffer) const {
    size_t i = 0;
    uint32_t game_id = htonl(game_id_);
    memcpy(buffer, &game_id, 4);
    i = 4;
    for (auto &ev: events_) {
        char ev_buffer[EVENT_BUFFER_SIZE];
        size_t len = ev->ToServerBuffer(ev_buffer);
        memcpy(buffer + i, ev_buffer, len);
        i += len;
    }
    return i;
}

std::shared_ptr<ServerData> ServerData::New(char *buffer, size_t len) {
    uint16_t ptr = 0;
    uint32_t game_id;
    memcpy(&game_id, buffer, 4);
    ptr = 4;
    std::vector<EventPtr> events;
    while (ptr < len) {
        uint32_t event_len2;
        memcpy(&event_len2, buffer + ptr, 4);
        uint32_t event_len = ntohl(event_len2);
        EventPtr e = Event::NewEvent(buffer + ptr, event_len + 8);
        ptr += event_len + 8;
        if (e) {
            events.push_back(e);
        } else {
            break;
        }
    }
    return std::make_shared<ServerData>(ntohl(game_id), std::move(events));
}

