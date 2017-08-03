#include <cstdint>
#include <unordered_set>
#include <set>

#include "siktacka.h"
#include "data_structures.h"

#ifndef SERVER_H
#define SERVER_H

#endif //SERVER_H


class PlayerData {
public:

    PlayerData(const std::string &player_name, const double &head_x, const double &head_y, uint32_t dir) :
            _player_name(player_name), head_x_(head_x), head_y_(head_y), move_dir_(dir) {};

    std::string player_name() const { return _player_name; }

    double head_x() const { return head_x_; }

    double head_y() const { return head_y_; }

    void set_dir(int8_t dir) { turn_dir_ = dir; }

    void move(uint32_t turningSpeed);

    std::pair<int, int> pixel() const { return std::make_pair((int)head_x, (int)head_y_); }

    bool active() const { return active_; }


private:
    std::string _player_name;
    double head_x_;
    double head_y_;
    int8_t turn_dir_;
    uint32_t move_dir_;
    bool active_;
};

using player_ptr = std::shared_ptr<PlayerData>;

class GameState {
public:
    GameState(time_t rand_seed, uint32_t maxx, uint32_t maxy, uint32_t turningSpeed);

    void new_player(const std::string &p);

    bool exist_player(const std::string &player_name) const;

    std::vector<event_ptr> nextTurn();


private:
    uint32_t gid_;
    uint32_t maxx_;
    uint32_t maxy_;
    std::set<player_ptr, SortPlayers> players_;
    std::vector<event_ptr> events_;
    bool active_ = false;
    uint32_t turningSpeed_;

    //przechowuje informacje o stanie planszy
    //byc moze unordered_map jesli trzeba bedzie trzymac gracza
    std::unordered_set<std::pair<uint32_t, uint32_t>> board_;

    Random rand_;

    uint32_t rand() { return rand_.next(); }

    struct SortPlayers;

    event_ptr newPlayer(const std::string &player_name);

    void startGame();
};


class Random {
public:
    Random(time_t seed) : random_(seed) {}

    uint32_t next() {
        random_ = (random_ * multipl_) % modulo_;
        return (uint32_t)random_;
    }

private:
    time_t random_;
    const int64_t multipl_ = 279470273;
    const int64_t modulo_ = 4294967291;
};



