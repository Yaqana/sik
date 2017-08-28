#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "siktacka.h"
#include "events.h"

class ClientData {
public:
    static std::shared_ptr<ClientData> New(char *buffer, size_t len);

    ClientData(uint64_t session_id, int8_t turn_direction,
               uint32_t next_event, const std::string &player_name);

    size_t ToBuffer(char *buffer);

    uint64_t session_id() const { return session_id_; }

    int16_t turn_direction() const { return turn_direction_; }

    uint32_t next_event() const { return next_event_; }

    std::string player_name() const { return player_name_; }

    uint32_t game_id() const { return game_id_; }

    void set_turn_direction(
            int8_t turn_direction) { turn_direction_ = turn_direction; }

    void inc_next_event() { next_event_++; }

    void reset() {
        next_event_ = 0;
        game_id_ = 0;
    }

    void set_game_id(uint32_t game_id) { game_id_ = game_id; }

private:
    uint64_t session_id_;
    int8_t turn_direction_;
    uint32_t next_event_;
    std::string player_name_;
    uint32_t game_id_;

};

using ClientDataPtr = std::shared_ptr<ClientData>;

class ServerData {
public:
    static std::shared_ptr<ServerData> New(char *buffer, size_t len);

    ServerData(uint32_t game_id, const std::vector<EventPtr> &events) :
            game_id_(game_id), events_(events) {};

    size_t ToBuffer(char *buffer) const;

    std::vector<EventPtr> events() const { return events_; }

    uint32_t game_id() const { return game_id_; }

private:
    uint32_t game_id_;
    std::vector<EventPtr> events_;
};

using ServerDataPtr = std::shared_ptr<ServerData>;

#endif // DATA_STRUCTURES_H
