#ifndef __DATATHREAD__
#define __DATATHREAD__

#include "clientInfo.h"

#define MAX_DATA_THREADS 64

// 每个上传/下载线程监听的参数
typedef struct {
    char ip[32];
    int  port;
    int  index;
} dataThreadArg_t;

// 上传/下载线程入口
void *dataThreadFunc(void *arg);

#endif
