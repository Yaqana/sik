#include <cstdint>
#include <set>
#include <map>
#include <netinet/in.h>

#include "siktacka.h"
#include "data_structures.h"

#ifndef SERVER_H
#define SERVER_H

#endif //SERVER_H

class Client{
public:
    Client(const struct sockaddr_in &addres, int64_t session_id):
            addres_(addres), session_id_(session_id) {};
    void sendTo(char *buffer, size_t len, int sock) const;
    int64_t session_id() const { return session_id_; }

private:
    struct sockaddr_in addres_;
    int64_t session_id_;
};

class SendData {
public:
    SendData(char* buffer, size_t len, const Client& client, int sock):
            buffer_(buffer), len_(len), client_(client), sock_(sock) {}
    void send() const { client_.sendTo(buffer_, len_, sock_);};
private:
    char* buffer_;
    size_t len_;
    const Client &client_;
    int sock_;
};

class PlayerData {
public:

    PlayerData(const std::string &player_name, const double &head_x, const double &head_y, uint32_t dir) :
            _player_name(player_name), head_x_(head_x), head_y_(head_y), move_dir_(dir) {};

    std::string player_name() const { return _player_name; }

    double head_x() const { return head_x_; }

    double head_y() const { return head_y_; }

    void set_dir(int8_t dir) { turn_dir_ = dir; }

    void move(uint32_t turningSpeed);

    std::pair<int, int> pixel() const { return std::make_pair((int)head_x_, (int)head_y_); }

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


class GameState {
public:
    GameState(time_t rand_seed, uint32_t maxx, uint32_t maxy, uint32_t turningSpeed);

    bool exist_player(const std::string &player_name) const;

    std::vector<event_ptr> nextTurn();

    void processData(cdata_ptr data);

    sdata_ptr eventsToSend(uint32_t firstEvent);

    bool active() const { return active_; }


private:
    struct SortPlayers {
        int operator()(const player_ptr p1, const player_ptr p2) const {
            return p1->player_name().compare(p2->player_name());
        }
    };
    uint32_t gid_;
    uint32_t maxx_;
    uint32_t maxy_;
    std::map<std::string, player_ptr> players_;
    std::vector<event_ptr> events_;
    bool active_ = false;
    uint32_t turningSpeed_;
    std::set<std::string> ready_players_;

    //przechowuje informacje o stanie planszy
    //byc moze unordered_map jesli trzeba bedzie trzymac gracza
    std::set<std::pair<uint32_t, uint32_t>> board_;

    Random rand_;

    uint32_t rand() { return rand_.next(); }

    player_ptr newPlayer(const std::string &player_name);



    void startGame();
};



