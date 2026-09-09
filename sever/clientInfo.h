// clientInfo.h

#ifndef CLIENT_INFO_H
#define CLIENT_INFO_H

typedef struct {
    int netfd;
    char home[1024];
    char cwd[1024];
    char name[64];
} clientInfo_t;

#endif
