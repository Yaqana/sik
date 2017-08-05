#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

/*
typedef struct client_data {
public:
    uint64_t session_id;
    int8_t turn_direction;
    uint32_t next_expected_event_no;
    std::string player_name;
} cdata;
 */

class ClientData {
public:
    ClientData(uint64_t session_id, int8_t turn_direction,
        uint32_t next_event, const std::string &player_name);
    uint64_t session_id() const { return _session_id; }
    int8_t turn_direction() const { return _turn_direction; }
    uint32_t next_event() const { return _next_event; }
    std::string player_name() const { return _player_name; }
    size_t toBuffer(char* buffer);
    void set_turn_direction(int8_t turn_direction) { _turn_direction = turn_direction; }
    void set_next_event(uint32_t next_event)  { _next_event = next_event; }
private:
    uint64_t _session_id;
    int8_t _turn_direction;
    uint32_t _next_event;
    std::string _player_name;
};

typedef std::shared_ptr<ClientData> cdata_ptr;

/*
typedef struct _event {
    uint32_t len;
    uint32_t event_no;
    uint8_t event_type;
    std::string player_name;

    //event data:
    uint32_t maxx;
    uint32_t maxy;
    std::vector<std::string> players;

    uint8_t player_number;
    uint32_t x;
    uint32_t y;

    uint32_t crc32;

} event; */

class Event {
public:
    virtual size_t toGuiBuffer(char* buffer) = 0;
    uint32_t event_no() { return event_no_; }
protected:
    uint32_t len_; // czy potrzebne?
    uint32_t event_no_;
    uint32_t crc32_;
};

class NewGame: public Event {
public:
    NewGame(char* buffer, size_t len, uint32_t event_no);
    NewGame(uint32_t maxx, uint32_t maxy, const std::vector<std::string> &players, uint32_t event_no);
    size_t toGuiBuffer(char* buffer);
private:
    uint32_t maxx_;
    uint32_t maxy_;
    std::vector<std::string> players_;
};

class Pixel: public Event {
public:
    Pixel(char* buffer, size_t len, uint32_t event_no);
    size_t toGuiBuffer(char* buffer);
private:
    uint32_t _x;
    uint32_t _y;
    uint8_t _playerNumber;
};

class PlayerEliminated: public Event {
public:
    PlayerEliminated(char* buffer, size_t len, uint32_t event_no);
    size_t toGuiBuffer(char* buffer);
private:
    uint8_t _playerNumber;
};

class GameOver: public Event {
public:
    GameOver();
    size_t toGuiBuffer(char* buffer);
};

using event_ptr = std::shared_ptr<Event>;

class ServerData {
public:
    ServerData(uint32_t game_id, const std::vector<event_ptr> events):
            _game_id(game_id), _events(events) {};
    bool hasEvents() const { return _events.size() > 0; }
    uint32_t firstEvent() const { return hasEvents() ? _events[0]->event_no() : 0; }
    std::vector<event_ptr> events() const { return _events; }
private:
    uint32_t _game_id;
    std::vector<event_ptr> _events;
};

typedef  std::shared_ptr<ServerData> sdata_ptr;


event_ptr buffer_to_event(char *buffer, size_t len);
sdata_ptr buffer_to_server_data(char *buffer, size_t len);
cdata_ptr buffer_to_client_data(char* buffer, size_t len);

#endif //DATA_STRUCTURES_H
