#include"command.h"

int cdFunction(int sfd,command_t *command,char * argv[],int argc){
    if (argc <= 1 || argc > 2){
        printf ("Format error\n");
        return 0;
    }
    command->len = strlen(argv[1]);
    strcpy(command->data,argv[1]);
    
    sendCommand(sfd,command);

    char buf[1024] = {0};
    int ret = recvResponse(sfd,buf,sizeof(buf));
    write(STDOUT_FILENO,buf,ret);
    return 0;
}
