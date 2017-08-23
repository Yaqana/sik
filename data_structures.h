#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "siktacka.h"
#include "events.h"

class ClientData {
public:
    ClientData(uint64_t session_id, int8_t turn_direction,
        uint32_t next_event, const std::string &player_name);
    uint64_t session_id() const { return session_id_; }
    int16_t turn_direction() const { return turn_direction_; }
    uint32_t next_event() const { return next_event_; }
    std::string player_name() const { return player_name_; }
    size_t toBuffer(char* buffer);
    void set_turn_direction(int8_t turn_direction) { turn_direction_ = turn_direction; }
    void inc_next_event()  { next_event_++; }
private:
    uint64_t session_id_;
    int8_t turn_direction_;
    uint32_t next_event_;
    std::string player_name_;
};

typedef std::shared_ptr<ClientData> cdata_ptr;

class ServerData {
public:
    ServerData(uint32_t game_id, const std::vector<EventPtr> &events):
            game_id_(game_id), events_(events) {};
    bool hasEvents() const { return events_.size() > 0; }
    uint32_t firstEvent() const { return hasEvents() ? events_[0]->event_no() : 0; }
    std::vector<EventPtr> events() const { return events_; }
    size_t toBuffer(char* buffer) const;
private:
    uint32_t game_id_;
    std::vector<EventPtr> events_;
};

typedef  std::shared_ptr<ServerData> sdata_ptr;


EventPtr buffer_to_event(char *buffer, size_t len);
sdata_ptr buffer_to_server_data(char *buffer, size_t len);
cdata_ptr buffer_to_client_data(char* buffer, size_t len);

#endif //DATA_STRUCTURES_H
