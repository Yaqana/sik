#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <set>
#include <map>
#include <netinet/in.h>

#include "siktacka.h"
#include "data_structures.h"

const uint64_t kmicroseconds = 1000000;

namespace {
    void hexdump(char *buffer, size_t len) {
        printf("\n");
        for (size_t i = 0; i < len; i++) {
            printf("%02X ", *(buffer + i));
        }
        printf("\n");
    }
}

class PlayerData {
public:

    PlayerData(const std::string &player_name, uint8_t number, const double &head_x, const double &head_y, uint32_t dir) :
            player_name_(player_name), number_(number), head_x_(head_x), head_y_(head_y), move_dir_(dir) {};

    void Move(uint32_t turningSpeed);
    void Activate() { active_ = true; }
    void Disactivate() {active_ = false;}

    void set_turn_dir(int8_t turn_dir) { turn_dir_ = turn_dir; }
    void set_head_x(double x) { head_x_ = x; }
    void set_head_y(double y) { head_y_= y; }

    std::string player_name() const { return player_name_; }
    double head_x() const { return head_x_; }
    double head_y() const { return head_y_; }
    std::pair<int, int> pixel() const { return std::make_pair((int)head_x_, (int)head_y_); }
    bool active() const { return active_; }
    uint8_t number() const { return number_; }


private:
    std::string player_name_;
    uint8_t number_;
    double head_x_;
    double head_y_;
    int8_t turn_dir_;
    uint32_t move_dir_;
    bool active_ = false;
};

using PlayerPtr = std::shared_ptr<PlayerData>;

class Random {
public:
    Random(time_t seed) : random_(seed) {}

    uint32_t Next() {
        uint32_t res = (uint32_t )random_;
        random_ = (random_ * multipl_) % modulo_;
        return res;
    }

private:
    const uint64_t multipl_ = 279470273;
    const uint64_t modulo_ = 4294967291;
    time_t random_;
};


class GameState {
public:
    GameState(time_t rand_seed, uint32_t maxx, uint32_t maxy, uint32_t turningSpeed);

    void NextTurn();
    void ResetIfGameOver();
    void ProcessData(ClientDataPtr data);

    bool ExistPlayer(const std::string &player_name) const;
    std::vector<ServerDataPtr> EventsToSend(uint32_t firstEvent);
    bool active() const { return active_; }
    bool is_pending() const { return is_pending_; }

private:
    uint32_t Rand() { return rand_.Next(); }
    PlayerPtr NewPlayer(const std::string &player_name);
    void StartGame();
    void EliminatePlayer(PlayerPtr player);
    void EndGame();
    Random rand_;

    uint32_t gid_;
    uint32_t maxx_;
    uint32_t maxy_;
    uint32_t turningSpeed_;
    std::map<std::string, PlayerPtr> players_;
    std::vector<std::string> ready_players_; // Players who are ready to play.
    uint32_t active_players_number_; // Not yet eliminated players.
    std::vector<EventPtr> events_;
    bool active_ = false;
    bool game_over_ = false;
    std::set<std::pair<uint32_t, uint32_t>> board_;
    bool is_pending_ = false;
};


class Client{
public:
    Client(const struct sockaddr_in &addres, uint64_t session_id):
            addres_(addres), session_id_(session_id) {};

    void SendTo(ServerDataPtr server_data, int sock);

    uint64_t session_id() const { return session_id_; }
    uint32_t addr() const { return addres_.sin_addr.s_addr; }
    uint16_t port() const { return addres_.sin_port; }
    bool IsDisactive() const { return timestamp_ < GetTimestamp() - 2 * kmicroseconds; }
    uint32_t next_event_no() const { return next_event_no_; }

    void set_timestamp(int64_t timestamp) { timestamp_ = timestamp; }
    void set_next_event_no (uint32_t next_event_no) { next_event_no_ = next_event_no; }


private:

    struct sockaddr_in addres_;
    uint64_t session_id_;
    int64_t timestamp_; // last time client was active
    uint32_t next_event_no_;
};

#endif // SERVER_H

