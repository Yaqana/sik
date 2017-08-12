#include <algorithm>
#include <iostream>
#include <cassert>

#include "server.h"

#define NDEGREES 360

void PlayerData::move(uint32_t turningSpeed) {
    move_dir_ = (move_dir_ + turningSpeed * turn_dir_) % NDEGREES;
    head_x_ -= sin(move_dir_);
    head_y_ -= cos(move_dir_);
}

GameState::GameState(time_t rand_seed, uint32_t maxx, uint32_t maxy, uint32_t turningSpeed) :
        rand_(Random(rand_seed)), maxx_(maxx), maxy_(maxy), turningSpeed_(turningSpeed) {
    gid_ = rand();
}


bool GameState::exist_player(const std::string &player_name) const {
    for (auto &p : players_) {
        if (p.first == player_name)
            return true;
    }
    return false;
}

player_ptr GameState::newPlayer(const std::string &player_name) {
    player_ptr player = std::make_shared<PlayerData>(
            player_name, players_.size(), (double) (rand() % maxx_) + 0.5,
            (double) (rand() % maxy_) + 0.5, rand() % 360);
    players_.insert(std::make_pair(player_name, player));
    return player;
}

void GameState::startGame() {
    active_ = true;
    std::vector<std::string> players;
    for (auto &p: players_)
        players.push_back(p.first);
    event_ptr event(new NewGame(maxx_, maxy_, std::move(players), events_.size()));
    events_.push_back(event);
    for (auto &player : players_) {
        player_ptr p = player.second;
        p->activate();
        event_ptr pixel(new Pixel((uint32_t) p->head_x(), (uint32_t) p->head_y(), p->number(), events_.size()));
        events_.push_back(pixel);
    }
    active_players_ = players_.size();
}

void GameState::processData(cdata_ptr data) {
    auto it = players_.find(data->player_name());
    player_ptr player;
    if (it == players_.end()) {
        player = newPlayer(data->player_name());
    } else {
        player = it->second;
    }
    if (data->turn_direction() != 0) {
        auto ready = find(ready_players_.begin(), ready_players_.end(), data->player_name());
        if (ready == ready_players_.end()) {
            ready_players_.push_back(data->player_name());
            if (ready_players_.size() == 2) {
                startGame();
            }
        }
    }
    player->set_dir(data->turn_direction());
}

void GameState::eliminatePlayer(player_ptr player) {
    event_ptr ev(new PlayerEliminated(player->number(), events_.size()));
    events_.push_back(ev);
    std::cout<<" Player Eliminated "<<player->number()<<"\n";
    player->disactivate();
    active_players_--;
}

void GameState::endGame() {
    event_ptr game_over(new GameOver(events_.size()));
    events_.push_back(game_over);
    reset();
}

void GameState::reset() {
    active_ = false;
    for (auto &p: players_)
        p.second->disactivate();
    active_players_ = 0;
    ready_players_.clear();
    board_.clear();
    gid_ = rand();
    for (auto &p: players_) {
        player_ptr player = p.second;
        player->set_head_x((double)(rand() % maxx_) + 0.5);
        player->set_head_y((double)(rand() % maxy_) + 0.5);
        player->set_dir(rand() % NDEGREES);
    }
}

void GameState::nextTurn() {
    for (auto &p : players_) {
        player_ptr player = p.second;
       if (player->active()) {
            std::pair<int32_t, int32_t> oldCoord = player->pixel();
            player->move(turningSpeed_);
            std::pair<int32_t, int32_t> newCoord = player->pixel();
            if (newCoord.first < 0 || newCoord.first >= maxx_
                || newCoord.second < 0 || newCoord.second >= maxy_) {
                eliminatePlayer(player);
            } else if (oldCoord.first != newCoord.first || oldCoord.second != newCoord.second) {
                event_ptr ev(new Pixel((uint32_t) newCoord.first, (uint32_t) newCoord.second, player->number(),
                                       events_.size()));
                events_.push_back(ev);
                if (board_.find(newCoord) != board_.end())
                    eliminatePlayer(player);
                else
                    board_.insert(newCoord);
            }
           if (active_players_ < 2) {
               endGame();
               return;
           }
        }
    }
};

std::vector<sdata_ptr> GameState::eventsToSend(uint32_t firstEvent) {
    std::vector<sdata_ptr> res;
    uint32_t len = 4;
    std::vector<event_ptr > evs;
    for (uint32_t it=firstEvent; it < events_.size(); it++) {
        if (len += events_[it]->len() > SERVER_TO_CLIENT_SIZE) {
            res.push_back(sdata_ptr(new ServerData(gid_, std::move(evs))));
            len = 4;
            evs = {};
        }
        evs.push_back(events_[it]);
    }
    if (evs.size() > 0)
        res.push_back(sdata_ptr(new ServerData(gid_, std::move(evs))));
    return std::move(res);
}

void Client::sendTo(sdata_ptr server_data, int sock) const {
    char buffer[SERVER_TO_CLIENT_SIZE];
    size_t len = server_data->toBuffer(buffer);
    socklen_t snda_len = (socklen_t) sizeof(addres_);
    int flags = 0;
    ssize_t snd_len = sendto(sock, buffer, len, flags,
                             (struct sockaddr *) &addres_, snda_len);
    if (snd_len != len) {
        syserr("error in sending datagram to client socket");
    }
}