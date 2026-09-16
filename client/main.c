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

    // 同时监听与服务端的连接：服务端一旦关闭（如 30 秒不活跃被踢出），客户端立即感知并退出
    event.data.fd = sfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,sfd,&event);

    while (1){
        struct epoll_event readyEvent[1024];
        int readyNum = epoll_wait(epfd,readyEvent,1024,-1);

        for (int i = 0 ; i < readyNum ; ++i){
            int fd = readyEvent[i].data.fd;
            if (fd == STDIN_FILENO){
                //客户端发送消息
                send_Message(sfd);
            }
            else if (fd == sfd){
                // 服务端关闭了连接（例如 30 秒不活跃被踢出）
                char c;
                int r = recv(sfd, &c, 1, MSG_PEEK);
                if (r <= 0){
                    printf("\n与服务端的连接已断开（可能因长时间不活跃被踢出），客户端退出\n");
                    close(sfd);
                    exit(0);
                }
                // 协议为请求-响应，正常不会有多余数据；若有则读走丢弃，避免 epoll 反复触发
                char tmp[1024];
                recv(sfd, tmp, sizeof(tmp), 0);
            }
        }
    }

    return 0;

}
