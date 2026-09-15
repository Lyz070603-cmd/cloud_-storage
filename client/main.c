#include "function.h"
#include "funcsend_recv.h"
#include "login.h"

char g_username[64] = {0};
char g_serverIp[64] = {0};

int main (int argc , char *argv[]){
    ARGS_CHECK(argc,3);

    strncpy(g_serverIp, argv[1], sizeof(g_serverIp) - 1);
    g_serverIp[sizeof(g_serverIp) - 1] = '\0';

    //初始化客户端
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    int ret = connect(sfd,(struct sockaddr*)&addr,sizeof(addr));
    ERROR_CHECK(ret, -1, "connect");
    printf ("Connect succes!\n");
    
    ret = loginFunction(sfd);
    if (ret == -1){
        printf("ERROR\n");
        exit(1);
    }
    if (ret == 1){
        // 用户主动选择退出
        close(sfd);
        return 0;
    }
    // ret == 0：登录成功，继续进入命令循环
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
