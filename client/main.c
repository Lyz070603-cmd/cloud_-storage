#include <function.h>
#include "funcsend_recv.h"

int main (int argc , char *argv[]){
    ARGS_CHECK(argc,3);

    //初始化客户端
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    int ret = connect(sfd,(struct sockaddr*)&addr,sizeof(addr));
    ERROR_CHECK(ret, -1, "connect");
    printf ("Connect succes!\n");
    
    //开始设置监听集合
    int epfd = epoll_create(1);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;
    epoll_ctl(epfd,EPOLL_CTL_ADD,STDIN_FILENO,&event);

    while (1){
        struct epoll_event readyEvent[1024];
        int readyNum = epoll_wait(epfd,readyEvent,1024,-1);

        for (int i = 0 ; i < readyNum ; ++i){
            if (readyEvent[i].data.fd == STDIN_FILENO){
                //客户端发送消息
                send_Message(sfd);
            }
        }
    }

    return 0;

}
