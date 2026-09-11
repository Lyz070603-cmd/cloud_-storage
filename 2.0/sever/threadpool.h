#include <function.h>
#include "taskQueue.h"

//保存每个客户端的数据

typedef struct threadpool_s{
    pthread_t *tidArr;
    int tidNum;
    taskQueue_t taskQueue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int exitFlag;
}threadpool_t;


int initThreadpool(threadpool_t *threadpool);
int makeThread(threadpool_t *threadpool,int threadNum);
void * threadFunc (void *arg);
