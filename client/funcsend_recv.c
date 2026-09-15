#include "funcsend_recv.h"


int send_Message(int sfd){
    char buf[1024] = {0};
    int ret = read(STDIN_FILENO,buf,sizeof(buf));
    if (ret == 0){
        //客户端断开连接
        return 0;
    }
    int argc = 0;
    char *argv[MAX_ARGS] = {0};
    command_t command;
    memset(&command,0,sizeof(command));
    wordSegmentation(buf,&argc,argv,&command);

    switch(command.cmd){
    case CMD_CD:
       cdFunction(sfd,&command,argv,argc);

        break;
    case CMD_LS:

        lsFunction(sfd,&command,argv,argc);
        break;
    case CMD_PWD:
        pwdFunction(sfd,&command,argv,argc);

        break;
    case CMD_GETS:

        getsFunction(sfd,&command,argv,argc);
        break;
    
    case CMD_PUTS:
        putsFunction(sfd,&command,argv,argc);
        break;

    case CMD_REMOVE:
        removeFunction(sfd,&command,argv,argc);
        break;
    default:
        printf("Unknown command\n");
        break;
    }

    return 0;
}
