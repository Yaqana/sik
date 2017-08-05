#include <cmath>

#include "server.h"

#define NDEGREES 360

void PlayerData::move(uint32_t turningSpeed) {
    move_dir_ = (move_dir_ + turningSpeed) % NDEGREES;
    head_x_ += cos(move_dir_);
    head_y_ += sin(move_dir_);
}

GameState::GameState(time_t rand_seed, uint32_t maxx, uint32_t maxy, uint32_t turningSpeed):
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
            player_name, (double)(rand() % maxx_) + 0.5,
            (double)(rand() % maxy_) + 0.5, rand() % 360);
    players_.insert(std::make_pair(player_name, player));
    return player;
}

void GameState::startGame() {
    active_ = true;
    std::vector<std::string> players(ready_players_.begin(), ready_players_.end());
    event_ptr event(new NewGame(maxx_, maxy_, std::move(players), events_.size()));
    events_.push_back(event);
}

void GameState::processData(cdata_ptr data) {
    auto it = players_.find(data->player_name());
    player_ptr player;
    if (it == players_.end()){
        player = newPlayer(data->player_name());
    }
    else {
        player = it->second;
    }
    if (data->turn_direction() != 0) {
        auto ready = ready_players_.find(data->player_name());
        if (ready == ready_players_.end()) {
            ready_players_.insert(data->player_name());
            if (ready_players_.size() == 2)
                startGame();
        }
    }
    player->set_dir(data->turn_direction());
}

std::vector<event_ptr> GameState::nextTurn() {
    std::vector<event_ptr> eventsGenerated;
    for (auto &player : players_) {
        if (player.second->active()) {
            std::pair<int, int> oldCoord = player.second->pixel();
            player.second->move(turningSpeed_);
            std::pair<int, int> newCoord = player.second->pixel();
            if (newCoord.first < 0 || newCoord.first >= maxx_
                || newCoord.second < 0 || newCoord.second >= maxy_) {
                // TODO PLAYER_ELIMINATED
            } else if (oldCoord.first != newCoord.first || oldCoord.second != newCoord.second) {
                // TODO generuj pixel
            }
        }
    }
};

sdata_ptr GameState::eventsToSend(uint32_t firstEvent) {
    std::vector<event_ptr> events(events_.begin()+firstEvent, events_.end());
    sdata_ptr serverData(new ServerData(gid_, std::move(events)));
    return serverData;
}

void Client::sendTo(char *buffer, size_t len, int sock) const {
    socklen_t snda_len = (socklen_t)sizeof(addres_);
    int flags = 0;
    ssize_t snd_len = sendto(sock, buffer, len, flags,
                             (struct sockaddr *)&addres_, snda_len);
    if (snd_len != len){
        syserr("error in sending datagram to client socket");
    }
}