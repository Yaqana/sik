#include "siktacka.h"

uint64_t get_timestamp(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    long long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
    return (uint64_t)time_in_micros;
}

uint16_t validate_port(const std::string &port){
    if (port.size() > 5)
        fatal("zbyt dlugi numer portu\n");
    for (int i = 0; i < port.size(); i++) {
        if (!isdigit(port[i]))
            fatal("numer portu moze zawierac tylko cyfry\n");
    }
    uint16_t res = (uint16_t)atoi(port.c_str());
    if (res > 65535)
        fatal("zbyt duzy numer portu\n");
    return res;
}
