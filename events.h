#ifndef EVENTS_H
#define EVENTS_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

class Event {
public:
    static std::shared_ptr<Event> NewEvent(char* buffer, size_t len);

    size_t ToServerBuffer(char *buffer) const;
    virtual size_t ToGuiBuffer(char *buffer) const = 0;

    virtual void MapName(const std::vector<std::string> &players) {}

    uint32_t event_no() { return event_no_; }
    uint32_t len() const { return len_; }
    virtual std::vector<std::string> players() const { return std::vector<std::string>(); }
    virtual bool IsGameOver() const { return false; };
    virtual bool IsNewGame() const { return false; }

protected:
    virtual uint8_t EventType() const = 0;

    uint32_t len_ = 13;
    uint32_t event_no_;

private:
        virtual size_t DataToBuffer(char *buffer) const = 0;
};

using EventPtr = std::shared_ptr<Event>;

class NewGame: public Event {
public:
    static std::shared_ptr<NewGame> New(char* buffer, size_t len, uint32_t event_no);

    NewGame(uint32_t maxx, uint32_t maxy, const std::vector<std::string> &players, uint32_t event_no);

    bool IsNewGame() const override { return true; }

    size_t ToGuiBuffer(char *buffer) const override;
    std::vector<std::string> players() const { return players_; }

protected:
    uint8_t EventType() const override { return 0; }

private:
    size_t DataToBuffer(char *buffer) const override;

    uint32_t maxx_;
    uint32_t maxy_;
    std::vector<std::string> players_;
};

class Pixel: public Event {
public:
    static std::shared_ptr<Pixel> New(char* buffer, size_t len, uint32_t event_no);

    Pixel(uint32_t x, uint32_t y, uint8_t number, uint32_t event_no);
    Pixel() = delete;

    void MapName(const std::vector<std::string> &players) override ;
    size_t ToGuiBuffer(char *buffer) const override;

protected:
    uint8_t EventType() const override { return 1; }

private:
    size_t DataToBuffer(char *buffer) const override;

    uint32_t x_;
    uint32_t y_;
    uint8_t playerNumber_;
    std::string player_name_;
};

class PlayerEliminated: public Event {
public:
    static std::shared_ptr<PlayerEliminated> New(char* buffer, size_t len, uint32_t event_no);

    PlayerEliminated(uint8_t player_numer, uint32_t event_no);
    PlayerEliminated() = delete;

    void MapName(const std::vector<std::string> &players) override ;
    size_t ToGuiBuffer(char *buffer) const override;

protected:
    uint8_t EventType() const override { return 2; }
private:
    size_t DataToBuffer(char *buffer) const override;

    uint8_t playerNumber_;
    std::string player_name_;
};

class GameOver: public Event {
public:
    static std::shared_ptr<GameOver> New(uint32_t event_no);

    GameOver(uint32_t event_no) { event_no_ = event_no;};
    GameOver() = delete;

    size_t ToGuiBuffer(char *buffer) const;

    bool IsGameOver() const override { return true; }

protected:
    uint8_t EventType() const override { return 3; }

private:
    size_t DataToBuffer(char *buffer) const override;
};

#endif //_EVENTS_H
