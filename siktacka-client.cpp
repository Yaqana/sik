#include <algorithm>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <poll.h>
#include <string>
#include <lzma.h>
#include <cstring>
#include <iostream>
#include <zconf.h>
#include <queue>
#include <netinet/tcp.h>
#include "data_structures.h"

namespace {
    const int UI = 0;
    const int SERVER = 1;
    const int SEND_INTERVAL_ms = 20;
    const int SEND_INTERVAL_us = 20000;
    const int LEFT = -1;
    const int AHEAD = 0;
    const int RIGHT = 1;
    std::vector<std::string> players;
    std::queue<EventPtr> events;
    std::string player_name, server;
    uint16_t server_port = 12345;
    uint16_t ui_port = 12346;
    std::string ui_server = "localhost";
    ClientDataPtr cdata;
    int ui_sock;
    int server_sock;
    struct sockaddr_in server_address;

    void
    ParsePort(const std::string &to_parse, std::string *server,
              uint16_t *port) {
        unsigned long pos = to_parse.find(":");
        if (pos == std::string::npos) {
            *server = to_parse;
            return;
        }
        *server = to_parse.substr(0, pos);
        std::string p = to_parse.substr(pos + 1, to_parse.size());
        if (p.size() > 0)
            *port = ValidatePort(p);
    }

    void ParseArguments(int argc, char *argv[]) {
        player_name = argv[1];
        ParsePort(argv[2], &server, &server_port);
        if (argc > 3) {
            ParsePort(argv[3], &ui_server, &ui_port);
        }
    }

    int UiWrite() {
        if (events.empty()) {
            return 0;
        }
        char buffer[CLIENT_TO_UI_SIZE];
        EventPtr event = events.front();
        events.pop();
        size_t len = event->ToGuiBuffer(buffer);
        if (event->IsGameOver()) {
            cdata->reset();
        }
        if (write(ui_sock, buffer, len) != (ssize_t) len) {
            syserr("partial / failed write");
            return 1;
        }
        return 0;
    }

    int UiRead() {
        char buffer[UI_TO_CLIENT_SIZE];
        ssize_t len = read(ui_sock, buffer, UI_TO_CLIENT_SIZE - 1);
        if (len < 0) {
            syserr("read");
            return 1;
        }
        if (strncmp(buffer, "LEFT_KEY_DOWN\n", (size_t) len) == 0) {
            cdata->set_turn_direction(LEFT);
        } else if (strncmp(buffer, "RIGHT_KEY_DOWN\n", (size_t) len) == 0) {
            cdata->set_turn_direction(RIGHT);
        } else {
            cdata->set_turn_direction(AHEAD);
        }
        return 0;
    }

    int ServerSend(struct sockaddr_in *server_address) {
        int sflags = 0;
        char c_data[CLIENT_TO_SERVER_SIZE];
        size_t msg_len = cdata->ToBuffer(c_data);
        socklen_t snda_len = (socklen_t) sizeof(*server_address);
        ssize_t len = sendto(server_sock, c_data, msg_len, sflags,
                             (struct sockaddr *) server_address, snda_len);
        if (len != (ssize_t) msg_len) {
            syserr("partial / failed write");
            return 1;
        }
        return 0;
    }

    void ServerRead(int sock, struct sockaddr_in *server_address) {
        socklen_t rcva_len = (socklen_t) sizeof(server_address);
        int flags = 0;
        char buffer[SERVER_TO_CLIENT_SIZE];
        ssize_t len = recvfrom(sock, buffer, SERVER_TO_CLIENT_SIZE, flags,
                               (struct sockaddr *) &server_address, &rcva_len);
        if (len < 0)
            syserr("error on datagram from client socket");
        ServerDataPtr data = ServerData::New(buffer, (size_t) len);
        if (cdata->game_id() == 0) {
            cdata->set_game_id(data->game_id());
        } else if (cdata->game_id() != data->game_id()) {
            return; // Ignore datagram from other game.
        }
        for (auto &ev : data->events()) {
            if (ev->event_no() != cdata->next_event()) {
                // Inconsecutive sequence of events.
                continue;
            }
            if (ev->event_no() == cdata->next_event()) {
                cdata->inc_next_event();
                if (ev->IsNewGame()) {
                    players = ev->players();
                }
                ev->MapName(players);
                events.push(ev);
            }
        }
    }

    int TcpSocket(const std::string &server, uint16_t port) {
        int err;
        int sock;
        struct addrinfo ui_addr_hints;
        struct addrinfo *ui_addr_result;
        memset(&ui_addr_hints, 0, sizeof(struct addrinfo));
        ui_addr_hints.ai_family = AF_INET; // IPv4
        ui_addr_hints.ai_socktype = SOCK_STREAM;
        ui_addr_hints.ai_protocol = IPPROTO_TCP;
        err = getaddrinfo(
                server.c_str(),
                (std::to_string(port)).c_str(),
                &ui_addr_hints,
                &ui_addr_result);
        if (err == EAI_SYSTEM) { // system error
            syserr("TCP getaddrinfo: %s", gai_strerror(err));
        } else if (err != 0) { // other error (host not found, etc.)
            fatal("TCP getaddrinfo: %s", gai_strerror(err));
        }

        sock = socket(ui_addr_result->ai_family, ui_addr_result->ai_socktype,
                      ui_addr_result->ai_protocol);
        if (sock < 0)
            syserr("Creatrion of socket failed.");

        int flag = 1;
        int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,
                                sizeof(int));
        if (result < 0) {
            syserr("Setting socket options faield.");
        }

        if (connect(sock, ui_addr_result->ai_addr, ui_addr_result->ai_addrlen) <
            0)
            syserr("Connect with gui failed.");

        freeaddrinfo(ui_addr_result);
        return sock;
    }

    int UdpSocket() {
        struct addrinfo server_addr_hints;
        struct addrinfo *server_addr_result;

        // 'converting' host/port in string to struct addrinfo
        (void) memset(&server_addr_hints, 0, sizeof(struct addrinfo));
        server_addr_hints.ai_family = AF_INET; // IPv4
        server_addr_hints.ai_socktype = SOCK_DGRAM;
        server_addr_hints.ai_protocol = IPPROTO_UDP;
        server_addr_hints.ai_flags = 0;
        server_addr_hints.ai_addrlen = 0;
        server_addr_hints.ai_addr = NULL;
        server_addr_hints.ai_canonname = NULL;
        server_addr_hints.ai_next = NULL;
        if (getaddrinfo(server.c_str(), NULL, &server_addr_hints,
                        &server_addr_result) != 0) {
            syserr("UDP getaddrinfo");
        }

        server_address.sin_family = AF_INET; // IPv4
        server_address.sin_addr.s_addr =
                ((struct sockaddr_in *) (server_addr_result->ai_addr))->sin_addr.s_addr; // address IP
        server_address.sin_port = htons(server_port);

        freeaddrinfo(server_addr_result);

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            syserr("Creatrion of socket failed.");
        return sock;

    }

    void SendAndReceive() {
        //poll configuration
        struct pollfd client[2];
        client[SERVER].fd = server_sock;
        client[UI].fd = ui_sock;
        client[SERVER].events = POLLIN;
        client[UI].events = POLLIN;
        client[SERVER].revents = 0;
        client[UI].revents = 0;

        int64_t next_send = GetTimestamp() + SEND_INTERVAL_us;
        int to_wait = SEND_INTERVAL_ms;

        int ret;

        while (true) {
            client[UI].revents = 0;
            client[SERVER].revents = 0;
            ret = poll(client, 2, to_wait);
            if (ret < 1) {
                next_send = GetTimestamp() + SEND_INTERVAL_us;
                client[SERVER].events = POLLOUT;
                ret = 1;
                to_wait = SEND_INTERVAL_ms;
            } else {
                if (client[UI].revents & POLLIN) {
                    UiRead();
                    client[SERVER].events |= POLLOUT;
                    if (events.empty())
                        client[UI].events = POLLIN;
                    else
                        client[UI].events = POLLOUT;
                }
                if (client[UI].revents & POLLOUT) {
                    UiWrite();
                    if (events.empty())
                        client[UI].events = POLLIN;
                }
                if (client[SERVER].revents & POLLOUT) {
                    ServerSend(&server_address);
                    client[SERVER].events = POLLIN;
                }
                if (client[SERVER].revents & POLLIN) {
                    ServerRead(server_sock, &server_address);
                    client[UI].events |= POLLOUT;
                }
                to_wait = std::max((next_send - GetTimestamp()) / 1000,
                                   (int64_t) 0);
            }
        }
    }
} // namespace


int main(int argc, char *argv[]) {

    if (argc < 3)
        fatal("Too few parameters\n");

    ParseArguments(argc, argv);

    uint64_t session_id = (uint64_t) GetTimestamp();
    int8_t turn = 0;
    uint32_t next_event = 0;

    // UDP configuration
    server_sock = UdpSocket();

    // TCP configuration
    ui_sock = TcpSocket(ui_server, ui_port);

    //client data configuration
    cdata = std::make_shared<ClientData>
            (session_id, turn, next_event, player_name);

    SendAndReceive();

    return 0;
}