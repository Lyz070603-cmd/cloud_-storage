#include "threadpool.h"
#include "workFunction.h"

#ifndef __THREADPOOL__
#define __THREADPOOL__

int initThreadpool(threadpool_t *threadpool){
    threadpool->tidNum = 0;
    initQueue(&threadpool->taskQueue);
    pthread_mutex_init(&threadpool->mutex,0);
    pthread_cond_init(&threadpool->cond,0);
    threadpool->tidArr = NULL;
    threadpool->exitFlag = 0;

    return 0;
}

int makeThread(threadpool_t * threadpool,int threadNum){
    threadpool->tidNum = threadNum;
    threadpool->tidArr = (pthread_t *)malloc(threadNum * sizeof(pthread_t));

    for (int i = 0 ; i < threadNum ; ++i){
        pthread_create(&threadpool->tidArr[i],NULL,threadFunc,threadpool);
        printf ("create new thread tid = %ld\n",threadpool->tidArr[i]);
    }

    return 0;
}

void * threadFunc (void *arg){
    threadpool_t *threadpool = (threadpool_t*)arg;
    while(1){
        pthread_mutex_lock(&threadpool->mutex);
        if (threadpool->taskQueue.sizeQueue == 0){
            pthread_cond_wait(&threadpool->cond,&threadpool->mutex);

            //如果接受到退出指令，执行退出
            if (threadpool->exitFlag == 1){
                pthread_mutex_unlock(&threadpool->mutex);
                printf("begin exiting\n");
                return NULL;
            }
        }
        clientInfo_t *clientInfo = popQueue(&threadpool->taskQueue);
        printf("begin working\n");
        //开始正式工作
        pthread_mutex_unlock(&threadpool->mutex);

        workFunc(clientInfo);

        printf("over\n");
    }
    return 0;
}
#endif
