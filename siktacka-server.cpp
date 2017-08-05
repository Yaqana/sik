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

    std::unordered_map<unsigned long, std::shared_ptr<Client>> clients; // address - session_id

    std::shared_ptr<GameState> gstate;

    std::queue<SendData> toSend;

    uint32_t width = 800;
    uint32_t height = 600;
    uint16_t port = 12345;
    uint32_t speed = 50;
    uint32_t turning_speed = 6;
    time_t seed;
    int wait_time_us;
    int wait_time_ms;
    int sock;

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
        wait_time_us = 1000000 / speed;
        wait_time_ms = wait_time_us / 1000;
    }

    void udp_socket() {
        struct sockaddr_in my_address;
        my_address.sin_family = AF_INET; // IPv4
        my_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
        my_address.sin_port = htons(port);

        if (bind(sock, (struct sockaddr *) &my_address,
                 (socklen_t) sizeof(my_address)) < 0)
            syserr("bind");

    }

    void process_cdata(const struct sockaddr_in &player_address, cdata_ptr data) {
        std::cout<<data->player_name()<<" "<<data->next_event()<<" "<<data->session_id()<<"\n"; // TODO
        auto itr = clients.find(player_address.sin_addr.s_addr);

        // nowe gniazdo, klient o znanej nazwie
        if (itr == clients.end() && gstate->exist_player(data->player_name())) {
            std::cout<<"Istnieje już gracz o takim nicku\n";
            return;
        }

        // zbyt stare session_id
        if (itr->second->session_id() > data->session_id()) {
            std::cout<<"Nieaktualny datagram";
            return;
        }

        std::shared_ptr<Client> c;
        // poprawny nowy klient
        if (itr == clients.end() || itr->second->session_id() < data->session_id()) {
            c = std::make_shared<Client>(player_address, data->session_id());
            clients.insert(std::make_pair(player_address.sin_addr.s_addr, c));
            std::cout<<"Tworzenie nowego gracza\n";
        }
        else {
            c = itr->second;
        }

        gstate->processData(data);

        sdata_ptr events_to_send = gstate->eventsToSend(data->next_event());
        char buffer[SERVER_TO_CLIENT_SIZE];
        size_t len = events_to_send->toBuffer(buffer);
        SendData s(buffer, len, c, sock);
        toSend.push(std::move(s));

    }

    void read_from_client() {
        struct sockaddr_in client_address;
        socklen_t rcva_len;
        rcva_len = (socklen_t) sizeof(client_address);
        ssize_t len;
        char buffer[CLIENT_TO_SERVER_SIZE];
        int flags = 0;
        len = recvfrom(sock, buffer, sizeof(buffer), flags,
                       (struct sockaddr *) &client_address, &rcva_len);
        cdata_ptr c = buffer_to_client_data(buffer, len);
        process_cdata(client_address, c);
    }

    void write_to_client() {
        const SendData &sendData = toSend.front();
        toSend.pop();
        sendData.send();
    }

    void snd_and_recv(int sock) {
        struct sockaddr_in player_address;
        socklen_t snda_len, rcva_len;
        int flags, sflags;
        ssize_t len = 1;
        struct pollfd client;
        client.fd = sock;
        client.events = POLLIN;
        int ret, to_wait;
        to_wait = wait_time_ms;
        int64_t next_send = get_timestamp() + wait_time_us;
        while(true) { //TODO
            client.revents = 0;
            ret = poll(&client, 1, to_wait);
            if (ret < 1) {
                next_send = get_timestamp() + wait_time_us;
                if(gstate->active())
                    gstate->nextTurn();
                to_wait = wait_time_ms;
            }
            else {
                if (client.revents & POLLIN) {
                    read_from_client();
                }
                else if (client.revents & POLLOUT) {
                    write_to_client();
                }
                to_wait = std::max((next_send - get_timestamp())/1000, (int64_t )0);
            }
        }
    }

}
int main(int argc, char *argv[]) {

    seed = time(NULL);

    parse_arguments(argc, argv);

    gstate = std::make_shared<GameState>(seed, height, width, turning_speed);

    // udp configuration
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");
    udp_socket();

    snd_and_recv(sock);

    return 0;
}