#include <cstring>
#include <netinet/in.h>
#include "data_structures.h"

#define NEW_GAME 0
#define PIXEL 1
#define PLAYER_ELIMINATED 2
#define GAME_OVER 3

NewGame::NewGame(uint32_t maxx, uint32_t maxy, const std::vector<std::string> &players, uint32_t event_no)
        : maxx_(maxx), maxy_(maxy), players_(players) { event_no_ = event_no; };

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
size_t ClientData::toBuffer(char *buffer) {
    uint64_t s_id = htonll(_session_id);
    uint32_t event = htonl(_next_event);
    memcpy(buffer, &s_id, 8);
    memcpy(buffer + 8, &_turn_direction, 1); // nie trzeba zmieniac kolejnosci
    memcpy(buffer + 9, &event, 4);
    memcpy(buffer + 13, _player_name.c_str(), _player_name.size());
    memcpy(buffer + 13 + _player_name.size(), "\n", 1);
    return 14 + _player_name.size();
}

ClientData::ClientData(uint64_t session_id, int8_t turn_direction, uint32_t next_event,
                       const std::string &player_name):
    _session_id(session_id),
    _turn_direction(turn_direction),
    _next_event(next_event),
    _player_name(player_name) {};

/*
size_t client_data_to_buffer(cdata_ptr data, char *buffer) {
    uint64_t s_id = htonll(data->session_id);
    int8_t td = data->turn_direction;
    uint32_t event = htonl(data->next_expected_event_no);
    memcpy(buffer, &s_id, 8);
    memcpy(buffer + 8, &td, 1);
    memcpy(buffer + 9, &event, 4);
    memcpy(buffer + 13, data->player_name.c_str(), data->player_name.size());
    return 13 + data->player_name.size();
}
*/

cdata_ptr buffer_to_client_data(char *buffer, size_t len) {
    uint64_t s_id;
    uint8_t td;
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
    /*
    c_data->player_name = std::string(pname);
    c_data->session_id = ntohll(s_id);
    c_data->turn_direction = td;
    c_data->next_expected_event_no = ntohl(event); */
    return c_data;
}

/*
event_ptr buffer_to_event(char *buffer, size_t len) { // trzeba bedzie wywalic
    uint32_t ptr = 0;
    event_ptr e(new event);
    uint32_t ev_no = 0;
    memcpy(&ev_no, buffer + ptr, 4);
    e->event_no = ntohl(ev_no);
    ptr += 4;
    memcpy(&(e->event_type), buffer + ptr, 1);
    ptr += 1;
    uint32_t x;
    uint32_t y;
    uint32_t count = 0;
    switch (e->event_type) {
        case NEW_GAME:
            memcpy(&x, buffer + ptr, 4);
            ptr += 4;
            memcpy(&y, buffer + ptr, 4);
            ptr += 4;
            count = 0;
            e->maxx = ntohl(x);
            e->maxy = ntohl(y);
            while (ptr < len) {
                while (*(buffer + ptr + count) >= 33 && *(buffer + ptr + count) <= 126) {
                    count += 1;
                }
                std::string player;
                player.assign(buffer + ptr, count);
                count++;
                ptr += count;
                count = 0;
                e->players.push_back(player);
            }
            break;
        case PIXEL:
            memcpy(&(e->player_number), buffer + ptr, 1);
            ptr += 1;
            memcpy(&x, buffer + ptr, 4);
            e->x = ntohl(x);
            ptr += 4;
            memcpy(&y, buffer + ptr, 4);
            e->y = ntohl(y);
            ptr += 4;
            break;
        case PLAYER_ELIMINATED:
            memcpy(&(e->player_number), buffer + ptr, 2);
            ptr += 1;
            break;
        case GAME_OVER:
            break;
        default:
            break; //TODO błąd
    }
    //TODO crc
    return e;
}
*/

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
    event_type_=0;
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
    event_type_=1;
}

PlayerEliminated::PlayerEliminated(char *buffer, size_t len, uint32_t event_no) {
    event_no_ = event_no;
    memcpy(&playerNumber_, buffer, 1);
    event_type_=2;
}

GameOver::GameOver() { event_type_=3; };

event_ptr buffer_to_event(char* buffer, size_t len) {
    uint32_t ptr = 0;
    uint32_t ev_no = 0;
    uint32_t event_type = 0;
    memcpy(&ev_no, buffer + ptr, 4);
    ptr += 4;
    memcpy(&event_type, buffer + ptr, 1);
    ptr += 1;
    switch (event_type) {
        case NEW_GAME:  {
            return std::make_shared<NewGame>(buffer+5, len-5, ev_no);
        }
        case PIXEL: {
            return std::make_shared<Pixel>(buffer+5, len-5, ev_no);
        }
        case PLAYER_ELIMINATED: {
            return std::make_shared<PlayerEliminated>(buffer+5, len-5, ev_no);
        }
        case GAME_OVER: {
            return std::make_shared<GameOver>();
        }
        default: return nullptr;
    }
}

sdata_ptr buffer_to_server_data(char *buffer, size_t len) {
    uint16_t ptr = 0;
    uint32_t game_id;
    memcpy(&game_id, buffer, 4);
    ptr = 4;
    std::vector<event_ptr> events;
    while (ptr < len) {
        uint32_t event_len;
        memcpy(&event_len, buffer + ptr, 4);
        event_len = ntohl(event_len);
        char *ev = (char *) malloc(event_len + 4);
        ptr += 4;
        memcpy(ev, buffer + ptr, event_len + 4); // wczytuje pola event_ i crc
        ptr += event_len + 4;
        event_ptr e = buffer_to_event(ev, event_len);
        events.push_back(e);
        free(ev);
    }
    return std::make_shared<ServerData>(ntohl(game_id), std::move(events));
}


/* // trzeba bedzie wywalic
size_t event_to_gui_buffer(event_ptr event, char *buffer) {
    size_t ptr = 0;
    uint32_t x = 0;
    uint32_t y = 0;
    switch (event->event_type) {
        case NEW_GAME: {
            sprintf(buffer, "NEW_GAME %u %u ", event->maxx, event->maxy);
            unsigned long p = event->players.size();
            ptr = strlen(buffer);
//            printf("gracze: %d\n", p);
            for (int i = 0; i < p; i++) {
                std::string player = event->players[i];
                ptr += sprintf(buffer + ptr, "%s ", player.c_str());
            }
            sprintf(buffer + ptr, "\n");
            return strlen(buffer);
        }
        case PIXEL:
            sprintf(buffer, "PIXEL %u %u %s\n", event->x, event->y, event->player_name.c_str());
            return strlen(buffer);
        case PLAYER_ELIMINATED:
            sprintf(buffer, "PLAYER_ELIMINATED %s\n", event->player_name.c_str());
            return strlen(buffer);
        default:
            return 0;
    }
}

*/

size_t ServerData::toBuffer(char *buffer) const {
    size_t i = 0;
    uint32_t game_id = htonl(game_id_);
    memcpy(buffer, &game_id, 4);
    i = 4;
    for(auto &ev: events_) {
        char ev_buffer[EVENT_BUFFER_SIZE];
        size_t len = ev->toServerBuffer(ev_buffer);
        memcpy(buffer + i, &ev_buffer, len);
        i += len;
    }
    return i;
}

size_t Event::toServerBuffer(char *buffer) {
    uint32_t event_no = htonl(event_no_);
    memcpy(buffer+4, &event_no, 4);
    memcpy(buffer+8, &event_type_, 1);
    const size_t offset = 4;
    uint32_t len = 5;
    len += dataToBuffer(buffer+9);
    size_t len_hton = htonl(len);
    memcpy(buffer, &len_hton, 4);
    return len + 8; //TODO crc
}

size_t NewGame::dataToBuffer(char *buffer) {
    uint32_t len = 0;
    for (auto &p : players_) {
        const char* pname = p.c_str();
        memcpy(buffer+len, &pname, p.length());
        memcpy(buffer+len+1, "\0", 1);
        len += p.length() + 1;
    }
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
