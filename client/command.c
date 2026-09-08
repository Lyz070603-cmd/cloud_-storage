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
int pwdFunction(int sfd,command_t *command,char * argv[],int argc){
    if (argc >1){
        printf ("Format error\n");
        return 0;
    }

    command->len = 0;
    sendCommand(sfd,command);

    char buf[1024] = {0};
    int ret = recvResponse(sfd,buf,sizeof(buf));
    write(STDOUT_FILENO,buf,ret);
    return 0;
}
int lsFunction(int sfd,command_t *command,char * argv[],int argc){
    if (argc > 2){
        printf ("Format error\n");
        return 0;
    }
    else if (argc == 1){
        strcpy(command->data,".");

    }
    else {
        strcpy(command->data,argv[1]);
    }
    command->len = strlen(command->data);

    sendCommand(sfd,command);

    //收到客户端的消息
    char buf[1024] = {0};
    int ret = recvResponse(sfd,buf,sizeof(buf));
    write(STDOUT_FILENO,buf,ret);


    return 0;

}
int getsFunction(int sfd,command_t *command,char * argv[],int argc){
    if (argc != 2){
        printf("Format error\n");
        return 0;
    }

    strcpy(command->data,argv[1]);
    command->len = strlen(command->data);

    sendCommand(sfd,command);
    //接收服务器回复
    command_t response;
    int ret = recvCommand(sfd, &response);

    if (strcmp(response.data, "READY") != 0) {
        printf("This file does not exist\n");
        return 0;
    }

    if(ret <= 0){
        printf("server disconnect\n");
        return -1;
    }

    // 接收文件大小
    off_t filesize;
    recvAll(sfd, &filesize, sizeof(filesize));

    // 创建本地文件
    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);

    // 接收文件
    char buf[1024];

    while (filesize > 0) {
        int recvSize = sizeof(buf);

        if (filesize < recvSize) {
            recvSize = filesize;
        }

        recvAll(sfd, buf, recvSize);

        write(fd, buf, recvSize);

       filesize -= recvSize;
    }

    close(fd);

    //接收服务器最后的OK
    recvCommand(sfd, &response);

    printf("Download success\n");

    return 0;
}
int putsFunction(int sfd, command_t *command, char *argv[], int argc)
{
    if (argc != 2) {
        printf("Format error\n");
        return 0;
    }

    // 1. 打开本地文件
    int fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 0;
    }

    // 2. 获取文件大小
    struct stat st;

    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 0;
    }

    // 3. 发送 puts 命令
    strcpy(command->data, argv[1]);
    command->len = strlen(command->data);

    sendCommand(sfd, command);

    // 4. 等待服务器回复 READY
    command_t response;

    recvCommand(sfd, &response);

    if (strcmp(response.data, "READY") != 0) {
        printf("Server refused\n");
        close(fd);
        return 0;
    }

    // 5. 发送文件大小
    off_t filesize = st.st_size;

    sendAll(sfd, &filesize, sizeof(filesize));

    // 6. 发送文件内容
    char buf[1024];

    while (1) {
        int ret = read(fd, buf, sizeof(buf));

        if (ret == 0) {
            break;
        }

        sendAll(sfd, buf, ret);
    }

    close(fd);

    // 7. 等待服务器发送 OK
    recvCommand(sfd, &response);

    if (strcmp(response.data, "OK") == 0) {
        printf("Upload success\n");
    }

    return 0;
}
int removeFunction(int sfd, command_t *command, char *argv[], int argc)
{
    if (argc != 2) {
        printf("Format error\n");
        return 0;
    }

    // 设置命令
    command->cmd = CMD_REMOVE;

    // 设置文件名
    strcpy(command->data, argv[1]);
    command->len = strlen(command->data);

    // 发送命令
    sendCommand(sfd, command);

    // 等待服务器回复
    command_t response;
    recvCommand(sfd, &response);

    if (strcmp(response.data, "OK") == 0) {
        printf("Remove success\n");
    }
    else {
        printf("This file does not exist\n");
    }

    return 0;
}
