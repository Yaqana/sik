#include <cstdint>
#include <getopt.h>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <netinet/in.h>
#include <iostream>
#include <unordered_map>
#include "err.h"
#include "siktacka.h"
#include "server.h"

extern char *optarg;
extern int optind, opterr, optopt;


namespace {

    std::unordered_map<struct sockaddr_in, int64_t> clients; // socket - session_id

    std::shared_ptr<GameState> gstate;

    int32_t is_positive_int(char *string) {
        char *temp = string;
        while (*temp) {
            if (!isdigit(*temp))
                fatal("%s powinien byc liczba dodatnia \n", string);
        }
        return atoi(string);
    }

    void parse_arguments(int argc, char **argv, uint32_t &width, uint32_t &height,
                         uint16_t &port, uint32_t &speed, uint32_t &turning_speed,
                         time_t &seed
    ) {
        int opt;
        int p;
        while ((opt = getopt(argc, argv, "W:H:p:s:t:r:")) != -1) {
            switch (opt) {
                case 'W':
                    width = (uint32_t) is_positive_int(optarg);
                    break;
                case 'H':
                    height = (uint32_t) is_positive_int(optarg);
                    break;
                case 'p':
                    p = is_positive_int(optarg);
                    if (p > 65535)
                        fatal("niepoprawny port %d \n", p);
                    port = (uint16_t) p;
                    break;
                case 's':
                    speed = (uint32_t) is_positive_int(optarg);
                    break;
                case 't':
                    turning_speed = (uint32_t) is_positive_int(optarg);
                    break;
                case 'r':
                    seed = (uint32_t) is_positive_int(optarg);
                    break;
                default: /* '?' */
                    fatal("niepoprawna flaga %c \n", opt);
            }
        }

    }

    void udp_socket(int &sock, uint16_t port) {
        struct sockaddr_in my_address;
        my_address.sin_family = AF_INET; // IPv4
        my_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
        my_address.sin_port = htons(port);

        if (bind(sock, (struct sockaddr *) &my_address,
                 (socklen_t) sizeof(my_address)) < 0)
            syserr("bind");

    }

    void process_new_cdata(const struct sockaddr_in &player_address, cdata_ptr data) {
        auto client = clients.find(player_address);

        // nowe gniazdo, klient o znanej nazwie
        if (client == clients.end() && gstate->exist_player(data->player_name))
            return;

        // poprawny nowy klient
        if (client == clients.end() || client->second < data->session_id())
            clients.insert(std::make_pair(player_address, data->session_id()));

        // zbyt stare session_id
        else if (client->second > data->session_id())
            return;

        // zgłaszamy gotowość do gry
        if (!gstate->is_active() && data->turn_direction() != 0) {
            if (gstate->new_player(data->player_name())) { //TODO
            } // zaczyna sie nowa gra
        }
    }

    void snd_and_recv(int sock) {
        struct sockaddr_in player_address;
        socklen_t snda_len, rcva_len;
        int flags, sflags;
        ssize_t len = 1;
        while (len >= 0) { //TODO warunek
            char client_datagram[SERVER_SEND_SIZE];
            rcva_len = (socklen_t) sizeof(player_address);
            flags = 0;
            len = recvfrom(sock, client_datagram, sizeof(client_datagram), flags,
                           (struct sockaddr*) &player_address, &rcva_len);
            if (len < 0)
                syserr("error on datagram from client socket");
            else {
                /*(void) printf("read from socket: %zd bytes: %.*s\n", len,
                              (int) len, client_datagram);*/
                cdata_ptr data = buffer_to_client_data(client_datagram, len);
                std::cout<<"name: "<<data->player_name<<"\n";
                printf("diretion %d\n", data->turn_direction());
                printf("session %llu\n", data->session_id());
                printf("next %d\n", data->next_event());
            }
        }
    }

}
int main(int argc, char *argv[]) {

    uint32_t width = 800;
    uint32_t height = 600;
    uint16_t port = 12345;
    uint32_t speed = 50;
    uint32_t turning_speed = 6;
    time_t seed = time(NULL);

    parse_arguments(
            argc, argv, width, height, port, speed, turning_speed, seed);

    gstate = std::make_shared<GameState>(seed);

    // udp configuration
    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");
    udp_socket(sock, port);

    snd_and_recv(sock);

    return 0;
}