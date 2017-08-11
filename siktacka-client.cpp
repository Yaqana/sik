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
#include "data_structures.h"

namespace {
    const int UI = 0;
    const int SERVER = 1;
    const int SEND_INTERVAL_ms = 20; // co ile wysylany jest komunikat
    const int SEND_INTERVAL_us = 20000;
    const int LEFT = -1;
    const int AHEAD = 0;
    const int RIGHT = 1;
    std::vector<std::string> players;
    std::queue<event_ptr> events;
    std::string player_name, server;
    uint16_t server_port = 12345;
    uint16_t ui_port = 12346;
    std::string ui_server = "localhost";
    // TODO remove
    void hexdump(char* buffer, size_t len){
        for (size_t i = 0; i < len; i++){
            printf("%02X ", *(buffer+i));
        }
    }
    cdata_ptr cdata;

}

void parse_port(const std::string &to_parse, std::string &server, uint16_t &port){
    unsigned long pos = to_parse.find(":");
    if (pos == std::string::npos) {
        server = to_parse;
        return;
    }
    server = to_parse.substr(0, pos);
    std::string p = to_parse.substr(pos + 1, to_parse.size());
    if (p.size() > 0)
        port = validate_port(p);
}

int parse_arguments(int argc, char* argv[]){
    player_name = argv[1];
    parse_port(argv[2], server, server_port);
    if(argc > 3) {
        parse_port(argv[3], ui_server, ui_port);
    }


    return 0;
}

int ui_write(int sock){
    if (events.empty()) {
        return 0;
    }
    char buffer[CLIENT_TO_UI_SIZE];
    event_ptr event = events.front();
    events.pop();
    size_t len = event->toGuiBuffer(buffer);
    if (write(sock, buffer, len) != len) {
        syserr("partial / failed write");
        return 1;
    }
    return 0;
}

int ui_read(int sock, char* buffer){
    memset(buffer, 0, UI_TO_CLIENT_SIZE);
    ssize_t len = read(sock, buffer, UI_TO_CLIENT_SIZE - 1);
    std::cout<<buffer;
    if (len < 0) {
        syserr("read");
        return 1;
    }
    if(strcmp(buffer, "LEFT_KEY_DOWN\n") == 0) {
        cdata->set_turn_direction(LEFT);
    }
    else if (strcmp(buffer, "RIGHT_KEY_DOWN\n")== 0) {
        cdata->set_turn_direction(RIGHT);
    }
    else
        cdata->set_turn_direction(AHEAD);
    return 0;
}

int server_send(int sock, struct sockaddr_in *server_address){
    int sflags = 0;
    char c_data[CLIENT_TO_SERVER_SIZE];
    size_t msg_len = cdata->toBuffer(c_data);
    socklen_t snda_len = (socklen_t) sizeof(*server_address);
    ssize_t len = sendto(sock, c_data, msg_len, sflags,
                         (struct sockaddr *) server_address, snda_len);
    if (len != msg_len) {
        syserr("partial / failed write");
        return 1;
    }
    return 0;
}

void server_read(int sock, struct sockaddr_in *server_address) {
    socklen_t rcva_len = (socklen_t) sizeof (server_address);
    int flags = 0;
    char buffer2[SERVER_TO_CLIENT_SIZE];
    ssize_t len = recvfrom(sock, buffer2, SERVER_TO_CLIENT_SIZE, flags,
                           (struct sockaddr *) &server_address, &rcva_len);
    if (len < 0)
        syserr("error on datagram from client socket");
    sdata_ptr data = buffer_to_server_data(buffer2, (size_t)len);
    for (auto &ev : data->events()) {
        if (ev->event_no() > cdata->next_event()) {
            std::cout << "niespojne zdarzenia" << ev->event_no() << " " << cdata->next_event() << "\n";
            return; // dostalismy niespojny kawalek zdarzen
        }
        if (ev -> event_no() == cdata->next_event()) {
            cdata->inc_next_event();
            if (players.size() == 0) {
                players = ev->players();
            }
            ev->mapName(players);
            events.push(ev);
        }
    }
}

int tcp_socket(const std::string &server, uint16_t port) {
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
        syserr("getaddrinfo: %s", gai_strerror(err));
    }
    else if (err != 0) { // other error (host not found, etc.)
        fatal("getaddrinfo: %s", gai_strerror(err));
    }

    sock = socket(ui_addr_result->ai_family, ui_addr_result->ai_socktype, ui_addr_result->ai_protocol);
    if (sock < 0)
        syserr("socket");

    if (connect(sock, ui_addr_result->ai_addr, ui_addr_result->ai_addrlen) < 0)
        syserr("connect");

    freeaddrinfo(ui_addr_result);
    return sock;
}

int udp_socket(
        const std::string &server,
        uint16_t port,
        struct sockaddr_in *my_address,
        struct sockaddr_in *srvr_address)
{
    struct addrinfo server_addr_hints;
    struct addrinfo *server_addr_result;

    int flags, sflags;
    size_t server_len;
    ssize_t snd_len, rcv_len;
    socklen_t rcva_len;
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
    if (getaddrinfo(server.c_str(), NULL, &server_addr_hints, &server_addr_result) != 0) {
        syserr("getaddrinfo");
    }

    my_address->sin_family = AF_INET; // IPv4
    my_address->sin_addr.s_addr =
            ((struct sockaddr_in*) (server_addr_result->ai_addr))->sin_addr.s_addr; // address IP
    my_address->sin_port = htons(port); // port from the command line

    freeaddrinfo(server_addr_result);

    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");
    return sock;

}


int main(int argc, char* argv[]) {
    /* parse arguments */
    if (argc < 3)
        fatal("za malo argumentow\n");

    int ret = parse_arguments(argc, argv);
    if (ret < 0)
        fatal("Invalid arguments");
    std::cout<<player_name<<" "<<server<<" "<<server_port<<" "<<ui_server<<" "<<ui_port<<"\n";

    uint64_t session_id = get_timestamp();
    int8_t turn = 0;
    uint32_t next_event = 0;

    // UDP configuration
    struct sockaddr_in server_address;
    struct sockaddr_in srvr_address;
    int server_sock = udp_socket(server, server_port, &server_address, &srvr_address);

    // TCP configuration
    int ui_sock = tcp_socket(ui_server, ui_port);

    //client data configuration
     cdata = std::make_shared<ClientData>
            (session_id, turn, next_event, player_name);

    //poll configuration
    struct pollfd client[2];
    client[SERVER].fd = server_sock;
    client[UI].fd = ui_sock;
    client[SERVER].events = POLLIN;
    client[UI].events = POLLIN;
    client[SERVER].revents = 0;
    client[UI].revents = 0;

    int active_players = 0;
    int64_t next_send = get_timestamp() + SEND_INTERVAL_us;
    int to_wait = SEND_INTERVAL_ms;

    char buffer_ui_receive[UI_TO_CLIENT_SIZE];

    ret = 1; //TODO sztuczna petla

    while(ret){ //TODO do kiedy pętla
        client[UI].revents = 0;
        client[SERVER].revents = 0;
        ret = poll(client, 2, to_wait);
        if(ret < 1) {
            next_send = get_timestamp() + SEND_INTERVAL_us;
            client[SERVER].events = POLLOUT;
            //server_send(server_sock, data, &server_address);
            ret=1; // TODO taka sztuczna petla
            to_wait=SEND_INTERVAL_ms;
        }
        else {
            if (client[UI].revents & POLLIN) {
                ui_read(ui_sock, buffer_ui_receive);
                client[SERVER].events |= POLLOUT;
                if (events.empty())
                    client[UI].events = POLLIN;
                else
                    client[UI].events = POLLOUT;
            }
            if (client[UI].revents & POLLOUT) {
                ui_write(ui_sock);
                if (events.empty())
                    client[UI].events = POLLIN;
            }
            if (client[SERVER].revents & POLLOUT) {
                server_send(server_sock, &server_address);
                client[SERVER].events = POLLIN;
                //std::cout<<get_timestamp()<<"\n";
            }
            if (client[SERVER].revents & POLLIN){
                server_read(server_sock, &server_address);
                client[UI].events |= POLLOUT;
                //client[SERVER].events = POLLOUT;
            }
            to_wait = std::max((next_send - get_timestamp())/1000, (int64_t)0);
        }
    }


    return 0;
}