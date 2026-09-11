#include "initTcp.h"

#ifndef __INITTCP__
#define __INITTCP__


int initTcp(char *ip , char * port){
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);

    int reuse = 1;
    int ret = setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    ERROR_CHECK(ret,-1,"setsockopt");

    bind(sfd,(struct sockaddr *)&addr,sizeof(addr));
    listen(sfd,20);

    return sfd;
    
}
#endif
