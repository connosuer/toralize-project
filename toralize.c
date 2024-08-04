#include "toralize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netdb.h>
#include <poll.h>
#include <dlfcn.h>
#include <arpa/inet.h>

#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)

#define TIMEOUT_SECONDS 10
#define POLL_TIMEOUT 5000 // 5 seconds in milliseconds

__attribute__((constructor))
static void toralize_init(void) {
    fprintf(stderr, "toralize.so loaded\n");
}

Req *request(struct sockaddr_in *sock2) { 
    DEBUG_PRINT("Creating request");
    Req *req; 

    req = malloc(reqsize);
    if (req == NULL) {
        DEBUG_PRINT("malloc failed: %s", strerror(errno));
        exit(1);
    }

    req->vn = 4;
    req->cd = 1;
    req->dstport = sock2->sin_port;
    req->dstip = sock2->sin_addr.s_addr; 
    strncpy((char *)req->userid, USERNAME, 8); 

    DEBUG_PRINT("Request created");
    return req; 
}

ssize_t read_with_timeout(int sockfd, void *buf, size_t len, int timeout_ms) {
    DEBUG_PRINT("Reading with timeout");
    struct pollfd fds;
    int ret;

    fds.fd = sockfd;
    fds.events = POLLIN;

    ret = poll(&fds, 1, timeout_ms);

    if (ret == -1) {
        DEBUG_PRINT("poll failed: %s", strerror(errno));
        return -1;
    } else if (ret == 0) {
        DEBUG_PRINT("read timeout");
        errno = ETIMEDOUT;
        return -1;
    }

    if (fds.revents & POLLIN) {
        ssize_t read_bytes = read(sockfd, buf, len);
        DEBUG_PRINT("Read %zd bytes", read_bytes);
        return read_bytes;
    }

    return -1;
}

int toralize_connect(int s2, const struct sockaddr *sock2, socklen_t addrlen) {
    DEBUG_PRINT("toralize_connect called with s2=%d", s2);
    (void)addrlen; // Suppress unused parameter warning
    int s;
    struct sockaddr_in sock; 
    struct timeval timeout;
    fd_set fdset;
    Req *req;
    Res *res; 
    char buf[ressize];
    int success; 
    int ret;
    int (*original_connect)(int, const struct sockaddr*, socklen_t);

    original_connect = dlsym(RTLD_NEXT, "connect");
    if (!original_connect) {
        DEBUG_PRINT("Error loading original connect: %s", dlerror());
        return -1;
    }

    DEBUG_PRINT("Attempting to connect to %u:%d", 
                ntohl(((struct sockaddr_in *)sock2)->sin_addr.s_addr), 
                ntohs(((struct sockaddr_in *)sock2)->sin_port));

    s = socket(AF_INET, SOCK_STREAM, 0); 
    if(s < 0){ 
        DEBUG_PRINT("socket creation failed: %s", strerror(errno));
        return -1;
    }

    sock.sin_family = AF_INET; 
    sock.sin_port = htons(PROXYPORT);
    if(inet_pton(AF_INET, PROXY, &sock.sin_addr) <= 0) {
        DEBUG_PRINT("Invalid proxy address or address not supported");
        close(s);
        return -1;
    }

    // Set non-blocking
    fcntl(s, F_SETFL, O_NONBLOCK);

    DEBUG_PRINT("Initiating connection to proxy %s:%d", PROXY, PROXYPORT);
    ret = original_connect(s, (struct sockaddr *)&sock, sizeof(sock));

    if (ret < 0) {
        if (errno == EINPROGRESS) {
            DEBUG_PRINT("Connection in progress. Waiting up to %d seconds...", TIMEOUT_SECONDS);
            timeout.tv_sec = TIMEOUT_SECONDS;
            timeout.tv_usec = 0;
            FD_ZERO(&fdset);
            FD_SET(s, &fdset);
            ret = select(s + 1, NULL, &fdset, NULL, &timeout);
            if (ret < 0) {
                DEBUG_PRINT("select failed: %s", strerror(errno));
                close(s);
                return -1;
            } else if (ret == 0) {
                DEBUG_PRINT("Connection timed out");
                close(s);
                return -1;
            } else {
                // Check if we're actually connected
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(s, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error != 0) {
                    DEBUG_PRINT("Connection failed after select: %s", strerror(so_error));
                    close(s);
                    return -1;
                }
            }
        } else {
            DEBUG_PRINT("connect failed: %s", strerror(errno));
            close(s);
            return -1;
        }
    }

    DEBUG_PRINT("Connected to proxy %s:%d", PROXY, PROXYPORT); 
    req = request((struct sockaddr_in *)sock2);
    write(s, req, reqsize);

    // Wait for the socket to become readable
    FD_ZERO(&fdset);
    FD_SET(s, &fdset);
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    ret = select(s + 1, &fdset, NULL, NULL, &timeout);
    if (ret < 0) {
        DEBUG_PRINT("select failed: %s", strerror(errno));
        free(req);
        close(s);
        return -1;
    } else if (ret == 0) {
        DEBUG_PRINT("Timeout waiting for proxy response");
        free(req);
        close(s);
        return -1;
    }

    memset(buf, 0, ressize);
    ret = read(s, buf, ressize);
    if(ret < 1) { 
        DEBUG_PRINT("read failed: %s", strerror(errno));
        free(req); 
        close(s);
        return -1; 
    }

    res = (Res *)buf;
    success = (res->cd == 90); 
    if(!success){ 
        DEBUG_PRINT("Unable to traverse the proxy, error code: %d", res->cd); 
        close(s); 
        free(req); 
        return -1; 
    }

    DEBUG_PRINT("Connected through the proxy");

    dup2(s, s2); 
    free(req); 

    return 0; 
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    DEBUG_PRINT("connect called with sockfd=%d", sockfd);
    static int (*real_connect)(int, const struct sockaddr *, socklen_t) = NULL;
    if (!real_connect) {
        DEBUG_PRINT("Loading real_connect function");
        real_connect = dlsym(RTLD_NEXT, "connect");
        if (!real_connect) {
            DEBUG_PRINT("Error loading real connect: %s", dlerror());
            return -1;
        }
    }
    
    if (getenv("TORALIZE_ACTIVE") == NULL) {
        DEBUG_PRINT("TORALIZE_ACTIVE not set, bypassing interception");
        return real_connect(sockfd, addr, addrlen);
    } else {
        DEBUG_PRINT("TORALIZE_ACTIVE is set");
    }
    
    DEBUG_PRINT("Calling toralize_connect");
    int result = toralize_connect(sockfd, addr, addrlen);
    DEBUG_PRINT("toralize_connect returned %d", result);
    return result;
}