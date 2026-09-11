#include "clientInfo.h"

typedef struct node_s{
    struct node_s *next;
    clientInfo_t *clientInfo;
}node_t;

typedef struct taskQueue_s{
    int sizeQueue;
    node_t *front;
    node_t *rear;
}taskQueue_t;

int initQueue(taskQueue_t *taskQueue);
int pushQueue(taskQueue_t *taskQueue, clientInfo_t * clientInfo);
clientInfo_t * popQueue(taskQueue_t *taskQueue);
