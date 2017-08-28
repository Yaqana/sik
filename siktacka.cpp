#include <boost/crc.hpp>
#include <sys/time.h>
#include "siktacka.h"

uint32_t GetCrc(char *buffer, size_t len) {
    boost::crc_32_type result;
    result.process_bytes(buffer, len);
    return (uint32_t) result.checksum();
}

int64_t GetTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
    return (int64_t) time_in_micros;
}

uint16_t ValidatePort(const std::string &port) {
    if (port.size() > 5)
        fatal("Port number to high\n");
    for (unsigned i = 0; i < port.size(); i++) {
        if (!isdigit(port[i]))
            fatal("%s is not a number\n", port.c_str());
    }
    uint16_t res = (uint16_t) atoi(port.c_str());
    if (res > 65535)
        fatal("Port number to high\n");
    return res;
}

