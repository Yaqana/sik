#include <algorithm>
#include <iostream>
#include <cassert>

#include "server.h"

#define NDEGREES 360

void Player::SendTo(ServerDataPtr server_data, int sock) const {
    char buffer[SERVER_TO_CLIENT_SIZE];
    size_t len = server_data->ToBuffer(buffer);
    socklen_t snda_len = (socklen_t) sizeof(addres_);
    int flags = 0;
    ssize_t snd_len = sendto(sock, buffer, len, flags,
                             (struct sockaddr *) &addres_, snda_len);
    if (snd_len != (ssize_t )len) {
        syserr("error in sending datagram to client socket");
    }
}

void Player::Move(uint32_t turningSpeed) {
    move_dir_ = (move_dir_ + turningSpeed * turn_dir_) % NDEGREES;
    head_x_ -= sin(move_dir_);
    head_y_ -= cos(move_dir_);
}

void Player::set_details(const std::string &player_name, uint8_t number, double head_x, double head_y, uint32_t move_dir) {
    player_name_ = player_name;
    number_ = number;
    head_x_ = head_x;
    head_y_ = head_y;
    move_dir_ = move_dir;
}

void Player::Reset(double x, double y, uint32_t move_dir) {
    move_dir_ = move_dir;
    head_x_ = x;
    head_y_ = y;
    had_game_over_ = false;
    active_ = false;
}

GameState::GameState(time_t rand_seed, uint32_t maxx, uint32_t maxy, uint32_t turningSpeed, int sock) :
        rand_(Random(rand_seed)), maxx_(maxx), maxy_(maxy), turningSpeed_(turningSpeed), sock_(sock) {
    gid_ = Rand();
}

void GameState::NextTurn() {
    for (auto &p : players_) {
        PlayerPtr player = p.second;
        if (player->active()) {
            std::pair<int32_t, int32_t> oldCoord = player->pixel();
            player->Move(turningSpeed_);
            std::pair<int32_t, int32_t> newCoord = player->pixel();

            if (newCoord.first <= 0 || newCoord.first >= (int32_t )maxx_
                || newCoord.second <= 0 || newCoord.second >= (int32_t )maxy_) {
                EliminatePlayer(player);
            } else if (oldCoord.first != newCoord.first || oldCoord.second != newCoord.second) {
                EventPtr ev(new Pixel((uint32_t) newCoord.first, (uint32_t) newCoord.second, player->number(),
                                      (uint32_t )events_.size()));
                events_.push_back(ev);
                if (board_.find(newCoord) != board_.end()) {
                    EliminatePlayer(player);
                } else {
                    board_.insert(newCoord);
                }
            }

            if (active_players_number_ < 2) {
                EndGame();
                return;
            }
        }
    }
};

void GameState::ProcessData(ClientDataPtr data, PlayerPtr player) {
    auto it = players_.find(data->player_name());
    if (it == players_.end()) {
        NewPlayer(data->player_name(), player);
    }
    if (data->turn_direction() != 0 && !active_) {
        auto ready = find(ready_players_.begin(), ready_players_.end(), data->player_name());
        if (ready == ready_players_.end()) {
            ready_players_.push_back(data->player_name());
            if (ready_players_.size() == players_.size() && players_.size() >= 2) {
                StartGame();
            }
        }
    }
    player->set_turn_dir(data->turn_direction());
    SendData(data->next_event(), player);
}

bool GameState::ExistPlayer(const std::string &player_name) const {
    for (auto &p : players_) {
        if (p.first == player_name)
            return true;
    }
    return false;
}

void GameState::SendData(uint32_t firstEvent, PlayerPtr player) {
    if (player->had_game_over()) {
        return;
    }
    std::vector<ServerDataPtr> res;
    uint32_t len = 4;
    std::vector<EventPtr > evs;
    for (uint32_t it=firstEvent; it < events_.size(); it++) {
        if (len += events_[it]->len() > SERVER_TO_CLIENT_SIZE) {
            player->SendTo(ServerDataPtr(new ServerData(gid_, std::move(evs))), sock_);
            len = 4;
            evs = {};
        }
        evs.push_back(events_[it]);
        if (events_[it]->IsGameOver()){
            player->set_had_game_over(true);
        }
    }
    if (evs.size() > 0) {
        player->SendTo(ServerDataPtr(new ServerData(gid_, std::move(evs))), sock_);
    }
    if (player->had_game_over()){
        if (++game_overs_sent_ == players_.size()){
            Reset();
        }
    }
}

void GameState::NewPlayer(const std::string &player_name, PlayerPtr player) {
    player->set_details( player_name, (uint8_t )players_.size(), (double) (Rand() % maxx_) + 0.5,
            (double) (Rand() % maxy_) + 0.5, Rand() % 360);
    players_.insert(std::make_pair(player_name, player));
}

void GameState::StartGame() {
    std::cout<<"new game starts\n";
    events_.clear();
    active_ = true;
    std::vector<std::string> players;
    for (auto &p: players_)
        players.push_back(p.first);
    EventPtr event(new NewGame(maxx_, maxy_, std::move(players), (uint32_t )events_.size()));
    events_.push_back(event);
    for (auto &player : players_) {
        PlayerPtr p = player.second;
        p->Activate();
        EventPtr pixel(new Pixel((uint32_t) p->head_x(), (uint32_t) p->head_y(), p->number(), (uint32_t )events_.size()));
        events_.push_back(pixel);
    }
    active_players_number_ = (uint32_t )players_.size();
}

void GameState::EliminatePlayer(PlayerPtr player) {
    EventPtr ev(new PlayerEliminated(player->number(), (uint32_t)events_.size()));
    events_.push_back(ev);
    player->Disactivate();
    active_players_number_--;
}

void GameState::EndGame() {
    EventPtr game_over(new GameOver((uint32_t )events_.size()));
    events_.push_back(game_over);
    active_ = false;
}

void GameState::Reset() {
    active_players_number_ = 0;
    game_overs_sent_ = 0;
    ready_players_.clear();
    board_.clear();
    gid_ = Rand();
    for (auto &p: players_) {
        p.second->Reset((double) (Rand() & maxx_) + 0.5,
                        (double) (Rand() % maxy_) + 0.5,
                        Rand() % NDEGREES
        );
    }
    events_.clear();
}
