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

class Client{
public:
    Client(const struct sockaddr_in &addres, int64_t session_id):
            addres_(addres), session_id_(session_id) {};
    void sendTo(sdata_ptr server_data, int sock) const;
    int64_t session_id() const { return session_id_; }
    uint32_t addr() const { return addres_.sin_addr.s_addr; }
    uint16_t port() const { return addres_.sin_port; }

private:
    struct sockaddr_in addres_;
    int64_t session_id_;
};

class SendData {
public:
    SendData(sdata_ptr server_data, std::shared_ptr<Client> client, int sock):
            server_data_(server_data), client_(client), sock_(sock) {
    }
    void send() const { client_->sendTo(server_data_, sock_);};
private:
    sdata_ptr server_data_;
    std::shared_ptr<Client> client_;
    int sock_;
};

class PlayerData {
public:

    PlayerData(const std::string &player_name, uint8_t number, const double &head_x, const double &head_y, uint32_t dir) :
            player_name_(player_name), number_(number), head_x_(head_x), head_y_(head_y), move_dir_(dir) {};

    std::string player_name() const { return player_name_; }

    double head_x() const { return head_x_; }

    double head_y() const { return head_y_; }

    void set_dir(int8_t dir) { turn_dir_ = dir; }

    void move(uint32_t turningSpeed);

    std::pair<int, int> pixel() const { return std::make_pair((int)head_x_, (int)head_y_); }

    bool active() const { return active_; }

    void activate() { active_ = true; }

    void disactivate() {active_ = false;}

    uint8_t number() const { return number_; }

    void set_head_x(double x) { head_x_ = x; }
    void set_head_y(double y) { head_y_= y; }


private:
    std::string player_name_;
    double head_x_;
    double head_y_;
    int8_t turn_dir_;
    uint32_t move_dir_;
    bool active_ = false;
    uint8_t number_;
};

using player_ptr = std::shared_ptr<PlayerData>;

class Random {
public:
    Random(time_t seed) : random_(seed) {}

    uint32_t next() {
        uint32_t res = (uint32_t )random_;
        random_ = (random_ * multipl_) % modulo_;
        return res;
    }

private:
    time_t random_;
    const uint64_t multipl_ = 279470273;
    const uint64_t modulo_ = 4294967291;
};


class GameState {
public:
    GameState(time_t rand_seed, uint32_t maxx, uint32_t maxy, uint32_t turningSpeed);

    bool exist_player(const std::string &player_name) const;

    void nextTurn();

    void processData(cdata_ptr data);

    std::vector<sdata_ptr> eventsToSend(uint32_t firstEvent);

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
    std::vector<EventPtr> events_;
    bool active_ = false;
    uint8_t active_players_;
    uint32_t turningSpeed_;
    std::vector<std::string> ready_players_;

    //przechowuje informacje o stanie planszy
    //byc moze unordered_map jesli trzeba bedzie trzymac gracza
    std::set<std::pair<uint32_t, uint32_t>> board_;

    Random rand_;

    uint32_t rand() { return rand_.next(); }

    player_ptr newPlayer(const std::string &player_name);

    void startGame();

    void eliminatePlayer(player_ptr player);

    void endGame();

    void reset();
};

#endif //SERVER_H

