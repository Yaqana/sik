#include <algorithm>
#include <cstdint>
#include <getopt.h>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <poll.h>
#include <queue>
#include "err.h"
#include "siktacka.h"
#include "server.h"

extern char *optarg;
extern int optind, opterr, optopt;

namespace {

    std::set<std::shared_ptr<Client>> clients;
    uint32_t had_game_over = 0;

    std::shared_ptr<GameState> gstate;

    uint32_t width = 800;
    uint32_t height = 600;
    uint16_t port = 12345;
    uint32_t speed = 50;
    uint32_t turning_speed = 6;
    time_t seed;
    int wait_time_us;
    int wait_time_ms;
    int sock;

    int32_t IsPositiveInt(char *string) {
        char *temp = string;
        while (*temp) {
            if (!isdigit(*temp))
                fatal("%s powinien byc liczba dodatnia \n", string);
            temp++;
        }
        return atoi(string);
    }

    void UpdateClients() {
        for (auto c: clients) {
            if (c->IsDisactive()) {
                clients.erase(c);
            }
            std::vector<ServerDataPtr> sdata_to_send = gstate->EventsToSend(c->next_event_no());
            for (auto &s: sdata_to_send){
                c->SendTo(s, sock);
            }
        }
    }

    void ParseArguments(int argc, char **argv) {
        int opt;
        int p;
        while ((opt = getopt(argc, argv, "W:H:p:s:t:r:")) != -1) {
            std::cout<<"nowa flaga: "<<opt<<"\n";
            switch (opt) {
                case 'W':
                    width = (uint32_t) IsPositiveInt(optarg);
                    break;
                case 'H':
                    height = (uint32_t) IsPositiveInt(optarg);
                    break;
                case 'p':
                    p = IsPositiveInt(optarg);
                    if (p > 65535)
                        fatal("niepoprawny port %d \n", p);
                    port = (uint16_t) p;
                    break;
                case 's':
                    speed = (uint32_t) IsPositiveInt(optarg);
                    break;
                case 't':
                    turning_speed = (uint32_t) IsPositiveInt(optarg);
                    break;
                case 'r':
                    seed = (uint32_t) IsPositiveInt(optarg);
                    break;
                default: /* '?' */
                    fatal("niepoprawna flaga %c \n", opt);
            }
        }
        wait_time_us = 1000000 / speed;
        wait_time_ms = wait_time_us / 1000;
    }

    void UdpSocket() {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            syserr("socket");
        struct sockaddr_in my_address;
        my_address.sin_family = AF_UNSPEC; // IPv4 and IPv6
        my_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
        my_address.sin_port = htons(port);

        if (bind(sock, (struct sockaddr *) &my_address,
                 (socklen_t) sizeof(my_address)) < 0)
            syserr("bind");

    }

    void ProcessCdata(const struct sockaddr_in &player_address, ClientDataPtr data) {

        bool new_client = true;
        std::shared_ptr<Client> client;

        // check if the client is new
        for (auto c: clients) {
            if (c->addr() == player_address.sin_addr.s_addr && c->port() == player_address.sin_port) {
                if (c->session_id() > data->session_id()) // Old datagram
                    return;
                new_client = false;
                client = c;
                break;
            }
        }

        // valid new client
        if (new_client) {
            client = std::make_shared<Client>(player_address, data->session_id());
            clients.insert(client);
        }

        client->set_timestamp(GetTimestamp());
        client->set_next_event_no(data->next_event());

        gstate->ProcessData(data);

        std::vector<ServerDataPtr> sdata_to_send = gstate->EventsToSend(data->next_event());
        for (auto &s: sdata_to_send){
            client->SendTo(s, sock);
        }

    }

    void ReadFromClient() {
        struct sockaddr_in client_address;
        socklen_t rcva_len;
        rcva_len = (socklen_t) sizeof(client_address);
        ssize_t len;
        char buffer[CLIENT_TO_SERVER_SIZE];
        int flags = 0;
        len = recvfrom(sock, buffer, sizeof(buffer), flags,
                       (struct sockaddr *) &client_address, &rcva_len);
        ClientDataPtr c = buffer_to_client_data(buffer, len);
        ProcessCdata(client_address, c);
    }

    void SendAndRecv() {
        struct pollfd client;
        client.fd = sock;
        client.events = POLLIN;
        int ret, to_wait;
        to_wait = wait_time_ms;
        int64_t next_send = GetTimestamp() + wait_time_us;
        while(true) { // TODO
            client.revents = 0;
            ret = poll(&client, 1, to_wait);
            if (ret < 1) {
                next_send = GetTimestamp() + wait_time_us;
                to_wait = wait_time_ms;
                if(gstate->active())
                    gstate->NextTurn();
                if (gstate->is_pending()) {
                    UpdateClients();
                }
                gstate->ResetIfGameOver();
            }
            else {
                if (client.revents & POLLIN) {
                    ReadFromClient();
                }
                    /*
                else if (client.revents & POLLOUT) {
                    write_to_client();
                    client.events = POLLIN;
                } */
                to_wait = std::max((next_send - GetTimestamp())/1000, (int64_t )0);
            }
        }
    }

}
int main(int argc, char *argv[]) {

    seed = time(NULL);

    ParseArguments(argc, argv);

    gstate = std::make_shared<GameState>(seed, width, height, turning_speed);

    UdpSocket();

    SendAndRecv();

    return 0;
}