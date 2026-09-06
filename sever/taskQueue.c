#include "taskQueue.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef __TASKQUEUE__
#define  __TASKQUEUE__

int initQueue(taskQueue_t *taskQueue){
    taskQueue->front = NULL;
    taskQueue->rear = NULL;
    taskQueue->sizeQueue = 0;
    return 0;
}

int pushQueue(taskQueue_t *taskQueue, clientInfo_t * clientInfo){
    
    node_t *p = (node_t *)malloc(sizeof(node_t));
    p->clientInfo = clientInfo;
    p->next = NULL;

    if (taskQueue->sizeQueue == 0){
        taskQueue->front = p;
        taskQueue->rear = p;
    }
    else {
        taskQueue->rear->next = p;
        taskQueue->rear = taskQueue->rear->next;
    }
    taskQueue->sizeQueue++;
    return 0;
}
clientInfo_t * popQueue(taskQueue_t * taskQueue){
    if (taskQueue->sizeQueue == 0){
        printf ("pop error\n");
        return NULL;
    }
    else {
        node_t *p = taskQueue->front;
        taskQueue->front = taskQueue->front->next;
        
        clientInfo_t *client = p->clientInfo;
        free(p);
        taskQueue->sizeQueue--;
        return client;
    }
}


#endif
