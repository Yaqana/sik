#include <cstdint>
#include <getopt.h>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <netinet/in.h>
#include <iostream>
#include <unordered_map>
#include <poll.h>
#include "err.h"
#include "siktacka.h"
#include "server.h"

extern char *optarg;
extern int optind, opterr, optopt;


namespace {

    std::unordered_map<struct sockaddr_in, int64_t> clients; // socket - session_id

    std::shared_ptr<GameState> gstate;

    uint32_t width = 800;
    uint32_t height = 600;
    uint16_t port = 12345;
    uint32_t speed = 50;
    uint32_t turning_speed = 6;
    time_t seed;
    int wait_time_us;
    int wait_time_ms;

    int32_t is_positive_int(char *string) {
        char *temp = string;
        while (*temp) {
            if (!isdigit(*temp))
                fatal("%s powinien byc liczba dodatnia \n", string);
        }
        return atoi(string);
    }

    void parse_arguments(int argc, char **argv) {
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
        wait_time_us = 1000000 / turning_speed;
        wait_time_ms = wait_time_us / 1000;
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
        if (client == clients.end() && gstate->exist_player(data->player_name()))
            return;

        // poprawny nowy klient
        if (client == clients.end() || client->second < data->session_id())
            clients.insert(std::make_pair(player_address, data->session_id()));

        // zbyt stare session_id
        else if (client->second > data->session_id())
            return;

    }

    void snd_and_recv(int sock) {
        struct sockaddr_in player_address;
        socklen_t snda_len, rcva_len;
        int flags, sflags;
        ssize_t len = 1;
        struct pollfd client;
        client.fd = sock;
        client.events = POLLIN | POLLOUT;
        int ret, to_wait;
        to_wait = wait_time_ms;
        uint64_t next_send = get_timestamp() + wait_time_us;
        while(true) { //TODO
            client.revents = 0;
            ret = poll(&client, 1, to_wait);
            if (ret < 1) {
                next_send = get_timestamp() + wait_time_us;
                //TODO wyslac wszystko co trzeba
                to_wait = wait_time_ms;
            }
            else {
                //TODO cos dostalismy
            }
        }
    }

}
int main(int argc, char *argv[]) {

    seed = time(NULL);

    parse_arguments(argc, argv);

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