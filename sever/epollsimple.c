#include "epollsimple.h"

#ifndef  __EPOLLSIMPLE__
#define  __EPOLLSIMPLE__
int epollAdd(int epfd , int netfd){
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = netfd;

    epoll_ctl(epfd,EPOLL_CTL_ADD,netfd,&event);
    return 0;
}

int epollDelet(int epfd, int netfd){
    epoll_ctl(epfd,EPOLL_CTL_DEL,netfd,NULL);
    return 0;
}

#endif
