#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "siktacka.h"
#include "events.h"

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

class ServerData {
public:
    ServerData(uint32_t game_id, const std::vector<event_ptr> &events):
            game_id_(game_id), events_(events) {};
    bool hasEvents() const { return events_.size() > 0; }
    uint32_t firstEvent() const { return hasEvents() ? events_[0]->event_no() : 0; }
    std::vector<event_ptr> events() const { return events_; }
    size_t toBuffer(char* buffer) const;
private:
    uint32_t game_id_;
    std::vector<event_ptr> events_;
};

typedef  std::shared_ptr<ServerData> sdata_ptr;


event_ptr buffer_to_event(char *buffer, size_t len);
sdata_ptr buffer_to_server_data(char *buffer, size_t len);
cdata_ptr buffer_to_client_data(char* buffer, size_t len);

#endif //DATA_STRUCTURES_H
