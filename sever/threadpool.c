#include "threadpool.h"
#include "threadworkflow.h"
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
        // 等待任务或退出指令（用 while 处理虚假唤醒）
        while (threadpool->taskQueue.sizeQueue == 0 && threadpool->exitFlag == 0){
            pthread_cond_wait(&threadpool->cond,&threadpool->mutex);
        }
        // 收到退出指令且没有剩余任务
        if (threadpool->exitFlag == 1 && threadpool->taskQueue.sizeQueue == 0){
            pthread_mutex_unlock(&threadpool->mutex);
            printf("begin exiting\n");
            return NULL;
        }
        clientInfo_t *clientInfo = popQueue(&threadpool->taskQueue);
        pthread_mutex_unlock(&threadpool->mutex);

        //开始连接数据库（不持有互斥锁）
        struct MYSQL *mysql = mysql_init(NULL);
        if (mysql == NULL) {
            fprintf(stderr, "mysql_init failed\n");
            close(clientInfo->netfd);
            free(clientInfo);
            continue;
        }
        if (mysql_real_connect(mysql,"localhost","root","Lyz610831","mydb",0,NULL,0) == NULL) {
            fprintf(stderr, "mysql_real_connect failed: %s\n", mysql_error(mysql));
            mysql_close(mysql);
            close(clientInfo->netfd);
            free(clientInfo);
            continue;
        }
        clientInfo->mysql = mysql;
        printf("begin working\n");

        thread_workflow(clientInfo);

        mysql_close(mysql);
        printf("over\n");
    }
    return 0;
}
#endif
