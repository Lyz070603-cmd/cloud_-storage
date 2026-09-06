#include "threadpool.h"
#include "initTcp.h"
#include "epollsimple.h"

int exitpipe[2] = {0};
void handler(int signum){
    write(exitpipe[1],"1",sizeof(char));
    return;
}
int main(int argc,char  *argv[]){
    ARGS_CHECK(argc,4);

    //父子进程建立管道通讯
    pipe(exitpipe);
    if (fork() != 0){
        //父进程监听信号
        signal(SIGUSR1,handler);
        wait(NULL);
        return 0;
    }
    
    //初始化服务端
    int sfd = initTcp(argv[1],argv[2]);

    //开始创建监听epoll
    int epfd = epoll_create(1);
    
    //监听加入服务器
    epollAdd(epfd,sfd);
    epollAdd(epfd,exitpipe[0]);

    //开始创建多线程
    int threadNum = atoi(argv[3]);
    threadpool_t threadpool;
    initThreadpool(&threadpool);
    makeThread(&threadpool,threadNum);

    //开始监听
    while(1){
        struct epoll_event readyEvent[1024];
        int readyNum = epoll_wait(epfd,readyEvent,1024,-1);

        for (int i = 0 ; i  < readyNum ; ++i){
            if (readyEvent[i].data.fd == sfd){
                //服务器接受客户端连接
                int netfd = accept(sfd,NULL,NULL);
                //创建一个节点分配任务
                clientInfo_t clientInfo;
                clientInfo.netfd = netfd;
                strcpy(clientInfo.home,"/");
                strcpy(clientInfo.cwd,"/");
                //加入任务队列
                pthread_mutex_lock(&threadpool.mutex);
                pushQueue(&threadpool.taskQueue,&clientInfo);
                printf ("accept new client\n");
                pthread_mutex_unlock(&threadpool.mutex);
                pthread_cond_signal(&threadpool.cond);
            }
            else if (readyEvent[i].data.fd == exitpipe[0]){
               //开始有序退出 
               threadpool.exitFlag = 1;
               pthread_cond_broadcast(&threadpool.cond);
               for (int  j = 0 ; j  < threadNum ; ++j){
                   pthread_join(threadpool.tidArr[j],NULL);
               }

               return 0;
            }
        }
    }


}
