#ifndef TORALIZE_H
#define TORALIZE_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PROXY "127.0.0.1"
#define PROXYPORT 9050
#define USERNAME "toraliz"

struct proxy_request {
    uint8_t vn; 
    uint8_t cd;
    uint16_t dstport; 
    uint32_t dstip;
    unsigned char userid[8];
};

typedef struct proxy_request Req;

struct proxy_response {
    uint8_t vn;
    uint8_t cd; 
    uint16_t _;
    uint32_t __; 
};

typedef struct proxy_response Res;

#define reqsize sizeof(struct proxy_request)
#define ressize sizeof(struct proxy_response)

Req *request(struct sockaddr_in *sock2);
int toralize_connect(int s2, const struct sockaddr *sock2, socklen_t addrlen);

#endif // TORALIZE_H