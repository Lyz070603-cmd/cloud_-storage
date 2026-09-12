// clientInfo.h

#ifndef CLIENT_INFO_H
#define CLIENT_INFO_H

#include <mysql/mysql.h>

typedef struct {
    int netfd;
    char home[1024];
    char cwd[1024];
    char name[64];
    struct MYSQL *mysql;
} clientInfo_t;

#endif
