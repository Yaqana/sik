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

void GameState::new_player(const std::string &p) {
    players_.insert(std::make_shared(
            new PlayerData(p, rand()%maxx_, rand()%maxy_, rand()%360)));
    if (players_.size() == 2) {
        active_ = true;
        startGame();
    }
}

bool GameState::exist_player(const std::string &player_name) const {
    for (auto &p : players_) {
        if (p->player_name() == player_name)
            return true;
    }
    return false;
}


struct GameState::SortPlayers {
    int operator()(const player_ptr p1, const player_ptr p2) const {
        return p1->player_name().compare(p2->player_name());
    }
};

event_ptr GameState::newPlayer(const std::string &player_name) {
    player_ptr player = std::make_shared<PlayerData>(
            player_name, (double)(rand() % maxx_) + 0.5,
            (double)(rand() % maxy_) + 0.5, rand() % 360);
    players_.insert(player);
    if (active_) {
        // TODO wygenerowac zdarzenie dla tego gracza
    }
    else if (players_.size() == 2) {
        startGame();
    }
}

void GameState::startGame() {
    active_ = true;
}

std::vector<event_ptr> GameState::nextTurn() {
    std::vector<event_ptr> eventsGenerated;
    for (auto &player : players_) {
        if (player->active()) {
            std::pair<int, int> oldCoord = player->pixel();
            player->move(turningSpeed_);
            std::pair<int, int> newCoord = player->pixel();
            if (newCoord.first < 0 || newCoord.first >= maxx_
                || newCoord.second < 0 || newCoord.second >= maxy_) {
                // TODO PLAYER_ELIMINATED
            } else if (oldCoord.first != newCoord.first || oldCoord.second != newCoord.second) {
                // TODO generuj pixel
            }
        }
    }
}

};