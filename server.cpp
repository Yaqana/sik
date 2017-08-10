#include <algorithm>
#include <iostream>

#include "server.h"

#define NDEGREES 360

void PlayerData::move(uint32_t turningSpeed) {
    move_dir_ = (move_dir_ + turningSpeed) % NDEGREES;
    head_x_ += cos(move_dir_);
    head_y_ += sin(move_dir_);
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
        event_ptr pixel(new Pixel((uint32_t) p->head_x(), (uint32_t) p->head_y(), p->number(), events_.size()));
        events_.push_back(pixel);
    }
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

void GameState::nextTurn() {
    for (auto &player : players_) {
        std::pair<int32_t, int32_t> oldCoord = player.second->pixel();
        player.second->move(turningSpeed_);
        std::pair<int32_t, int32_t> newCoord = player.second->pixel();
        if (newCoord.first < 0 || newCoord.first >= maxx_
            || newCoord.second < 0 || newCoord.second >= maxy_) {
            event_ptr ev(new PlayerEliminated(player.second->number(), events_.size()));
            events_.push_back(ev);
        } else if (oldCoord.first != newCoord.first || oldCoord.second != newCoord.second) {
            event_ptr ev(new Pixel((uint32_t) newCoord.first, (uint32_t) newCoord.second, player.second->number(),
                                   events_.size()));
            events_.push_back(ev);
        }
    }
};

sdata_ptr GameState::eventsToSend(uint32_t firstEvent) {
    if (firstEvent == events_.size())
        return sdata_ptr();
    std::vector<event_ptr> events(events_.begin() + firstEvent, events_.end());
    sdata_ptr serverData(new ServerData(gid_, std::move(events)));
    return serverData;
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