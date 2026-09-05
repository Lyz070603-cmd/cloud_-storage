#include "funcsend_recv.h"

#ifndef __FUNCTION__
#define __FUNCTION__

int receive_Information(int sfd){
    char buf[1024] = {0};
    int sret = read(sfd,buf,sizeof(buf));
    if (sret == 0){
        printf ("Server disconnected\n");
        close(sfd);
        exit(0);
    }
    write(STDOUT_FILENO,buf,sret); 
    
    return 0;
}

int send_Message(int sfd){
    char buf[1024] = {0};
    int rset = read(STDIN_FILENO,buf,sizeof(buf));
    if (rset == 0){
        printf ("Connection has been actively disconnected");
        close (sfd);
        exit(0);
    }
    write(sfd,buf,rset);
    return 0;
}
#endif
