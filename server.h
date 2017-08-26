#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <set>
#include <map>
#include <netinet/in.h>

#include "siktacka.h"
#include "data_structures.h"

namespace {
    void hexdump(char *buffer, size_t len) {
        printf("\n");
        for (size_t i = 0; i < len; i++) {
            printf("%02X ", *(buffer + i));
        }
        printf("\n");
    }
}

class Player {
public:

    Player (const struct sockaddr_in &addres, uint64_t session_id) :
            addres_(addres), session_id_(session_id) {};

    void Move(uint32_t turningSpeed);
    void Activate() { active_ = true; }
    void Disactivate() {active_ = false;}
    void SendTo(ServerDataPtr server_data, int sock) const;

    bool had_game_over() const { return had_game_over_; }
    uint64_t session_id() const { return session_id_; }
    uint32_t addr() const { return addres_.sin_addr.s_addr; }
    uint16_t port() const { return addres_.sin_port; }
    std::string player_name() const { return player_name_; }
    double head_x() const { return head_x_; }
    double head_y() const { return head_y_; }
    std::pair<int, int> pixel() const { return std::make_pair((int)head_x_, (int)head_y_); }
    bool active() const { return active_; }
    uint8_t number() const { return number_; }

    void set_had_game_over(bool had_game_over) { had_game_over_ = had_game_over; }
    void set_turn_dir(int8_t turn_dir) { turn_dir_ = turn_dir; }
    void Reset(double x, double y, uint32_t move_dir);
    void set_details(const std::string &player_name, uint8_t  number, double head_x, double head_y, uint32_t move_dir);


private:
    std::string player_name_;
    uint8_t number_;
    double head_x_;
    double head_y_;
    int8_t turn_dir_;
    uint32_t move_dir_;
    bool active_ = false;
    struct sockaddr_in addres_;
    uint64_t session_id_;
    bool had_game_over_;
};

using PlayerPtr = std::shared_ptr<Player>;

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
    GameState(time_t rand_seed, uint32_t maxx, uint32_t maxy, uint32_t turningSpeed, int sock);

    void NextTurn();
    void ProcessData(ClientDataPtr data, PlayerPtr player);

    bool ExistPlayer(const std::string &player_name) const;
    bool active() const { return active_; }


private:
    uint32_t Rand() { return rand_.Next(); }
    void SendData (uint32_t , PlayerPtr player);
    void NewPlayer(const std::string &player_name, PlayerPtr player);
    void StartGame();
    void EliminatePlayer(PlayerPtr player);
    void EndGame();
    void Reset();
    Random rand_;

    uint32_t gid_;
    uint32_t maxx_;
    uint32_t maxy_;
    uint32_t turningSpeed_;
    int sock_;
    std::map<std::string, PlayerPtr> players_;
    std::vector<std::string> ready_players_; // Players who are ready to play.
    uint32_t active_players_number_; // Not yet eliminated players.
    std::vector<EventPtr> events_;
    bool active_ = false;
    std::set<std::pair<uint32_t, uint32_t>> board_;
    uint32_t game_overs_sent_;
};


#endif // SERVER_H

