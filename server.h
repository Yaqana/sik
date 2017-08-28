#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <set>
#include <map>
#include <netinet/in.h>

#include "siktacka.h"
#include "data_structures.h"

const int64_t kmicroseconds = 1000000;

class PlayerData {
public:

    PlayerData(const std::string &player_name, uint8_t number) :
            player_name_(player_name), number_(number) {};

    void Move(uint32_t turningSpeed);

    void Activate() { active_ = true; }

    void Disactivate() { active_ = false; }

    void set_turn_dir(int8_t turn_dir) { turn_dir_ = turn_dir; }

    void set_head_x(double x) { head_x_ = x; }

    void set_head_y(double y) { head_y_ = y; }

    void set_move_dir(uint32_t move_dir) { move_dir_ = move_dir; }

    void set_number(uint8_t number) { number_ = number; }

    std::string player_name() const { return player_name_; }

    double head_x() const { return head_x_; }

    double head_y() const { return head_y_; }

    std::pair<int, int> pixel() const {
        return std::make_pair((int) head_x_, (int) head_y_);
    }

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
    Random(uint64_t seed) : random_(seed) {}

    uint64_t Next() {
        uint64_t res = random_;
        random_ = (random_ * multipl_) % modulo_;
        return res;
    }

private:
    const uint64_t multipl_ = 279470273;
    const uint64_t modulo_ = 4294967291;
    uint64_t random_;
};


class GameState {
public:
    GameState(uint64_t rand_seed, uint32_t maxx, uint32_t maxy,
              uint32_t turningSpeed);

    void NextTurn();

    void ResetIfGameOver();

    // Returns pointer to new client if one was created and nullptr otherwise.
    PlayerPtr ProcessData(ClientDataPtr data, PlayerPtr player);

    void RemovePlayer(PlayerPtr player);

    std::vector<ServerDataPtr> EventsToSend(uint32_t firstEvent);

    bool active() const { return active_; }

    bool is_pending() const { return is_pending_; }

private:
    uint64_t Rand() { return rand_.Next(); }

    PlayerPtr NewPlayer(const std::string &player_name);

    void StartGame();

    void EliminatePlayer(PlayerPtr player);

    void EndGame();

    void ResetPlayers();

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
    bool game_over_ = true;
    std::set<std::pair<uint32_t, uint32_t>> board_;
    bool is_pending_ = false;
};


class Client {
public:
    Client(const struct sockaddr_in &addres, uint64_t session_id) :
            addres_(addres), session_id_(session_id) {};

    void SendTo(ServerDataPtr server_data, int sock);

    uint64_t session_id() const { return session_id_; }

    uint32_t addr() const { return addres_.sin_addr.s_addr; }

    uint16_t port() const { return addres_.sin_port; }

    bool IsDisactive() const {
        return timestamp_ < (GetTimestamp() - 2 * kmicroseconds);
    }

    uint32_t next_event_no() const { return next_event_no_; }

    PlayerPtr player() const { return player_; }

    void set_timestamp(int64_t timestamp) { timestamp_ = timestamp; }

    void set_next_event_no(
            uint32_t next_event_no) { next_event_no_ = next_event_no; }

    void set_player(PlayerPtr player) { player_ = player; }


private:

    struct sockaddr_in addres_;
    uint64_t session_id_;
    int64_t timestamp_; // last time client was active
    uint32_t next_event_no_;
    PlayerPtr player_;
};

#endif // SERVER_H

