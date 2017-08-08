#ifndef EVENTS_H
#define EVENTS_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

class Event {
        public:
        virtual size_t toGuiBuffer(char* buffer) = 0;
        size_t toServerBuffer(char* buffer);
        uint32_t event_no() { return event_no_; }
protected:
        uint32_t len_; // czy potrzebne?
        uint32_t event_no_;
        uint32_t crc32_;
        virtual uint8_t eventType() = 0;
private:
        virtual size_t dataToBuffer(char* buffer) = 0;
};

class NewGame: public Event {
public:
    NewGame(char* buffer, size_t len, uint32_t event_no);
    NewGame(uint32_t maxx, uint32_t maxy, const std::vector<std::string> &players, uint32_t event_no);
    size_t toGuiBuffer(char* buffer) override;
protected:
    uint8_t eventType() override { return 0; }
private:
    uint32_t maxx_;
    uint32_t maxy_;
    std::vector<std::string> players_;
    size_t dataToBuffer(char* buffer) override;
};

class Pixel: public Event {
public:
    Pixel(char* buffer, size_t len, uint32_t event_no);
    size_t toGuiBuffer(char* buffer) override;
protected:
    uint8_t eventType() override { return 1; }
private:
    uint32_t x_;
    uint32_t y_;
    uint8_t playerNumber_;
    size_t dataToBuffer(char* buffer) override;
};

class PlayerEliminated: public Event {
public:
    PlayerEliminated(char* buffer, size_t len, uint32_t event_no);
    size_t toGuiBuffer(char* buffer) override;
protected:
    uint8_t eventType() override { return 2; }
private:
    uint8_t playerNumber_;
    size_t dataToBuffer(char* buffer) override;
};

class GameOver: public Event {
public:
    GameOver();
    size_t toGuiBuffer(char* buffer);
    size_t dataToBuffer(char* buffer) override;
protected:
    uint8_t eventType() override { return 3; }
};

using event_ptr = std::shared_ptr<Event>;
#endif //_EVENTS_H
