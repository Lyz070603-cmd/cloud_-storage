#include"command.h"
#include <openssl/sha.h>

// 计算打开文件的 SHA1，输出 40 位十六进制（hexout 至少 41 字节）
// 计算完成后把文件偏移复位到 0，方便后续再读
static int sha1_fd(int fd, char *hexout) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);

    unsigned char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        SHA1_Update(&ctx, buf, (size_t)n);
    }
    if (n == -1) {
        return -1;
    }

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &ctx);
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        sprintf(hexout + i * 2, "%02x", hash[i]);
    }
    hexout[40] = '\0';

    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        return -1;
    }
    return 0;
}

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
    if (ret <= 0) {
        return -1;
    }
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
    if (ret <= 0) {
        return -1;
    }
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
    if (ret <= 0) {
        return -1;
    }
    write(STDOUT_FILENO,buf,ret);


    return 0;

}

//客户端想要从服务端下载文件
int getsFunction(int sfd,command_t *command,char * argv[],int argc){
    if (argc != 2) {
        printf("Format error\n");
        return 0;
    }

    const char *filename = argv[1];

    // 构造 gets 命令
    memset(command, 0, sizeof(command_t));
    command->cmd = CMD_GETS;
    strcpy(command->data, filename);
    command->len = strlen(command->data);

    if (sendCommand(sfd, command) == -1) {
        perror("sendCommand");
        return -1;
    }

    // 等待服务器 READY
    char response[100] = {0};
    if (recvResponse(sfd, response, sizeof(response)) <= 0) {
        perror("recvResponse");
        return -1;
    }
    if (strcmp(response, "READY") != 0) {
        printf("Server response: %s\n", response);
        return 0;
    }

    // 接收文件总大小
    off_t filesize = 0;
    if (recvAll(sfd, &filesize, sizeof(filesize)) <= 0) {
        perror("recvAll filesize");
        return -1;
    }

    // 打开本地文件写
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    // 接收文件内容
    char buf[8192];
    off_t received = 0;
    while (received < filesize) {
        size_t n = sizeof(buf);
        if (filesize - received < (off_t)n) {
            n = (size_t)(filesize - received);
        }
        if (recvAll(sfd, buf, n) <= 0) {
            perror("recvAll file");
            close(fd);
            return -1;
        }
        if (write(fd, buf, n) != (ssize_t)n) {
            perror("write");
            close(fd);
            return -1;
        }
        received += (off_t)n;

        double percent = (double)received / filesize * 100;
        printf("\rDownload: %.2f%%", percent);
        fflush(stdout);
    }
    printf("\n");
    close(fd);

    // 等待服务器最终回复
    memset(response, 0, sizeof(response));
    if (recvResponse(sfd, response, sizeof(response)) <= 0) {
        perror("recvResponse");
        return -1;
    }
    if (strcmp(response, "OK") == 0) {
        printf("Download success\n");
    } else {
        printf("Download failed: %s\n", response);
    }
    return 0;
}

//客户端想要上传文件给服务端（支持秒传）
int putsFunction(int sfd, command_t *command, char *argv[], int argc)
{
    if (argc != 2) {
        printf("Format error\n");
        return 0;
    }

    const char *filename = argv[1];

    // 打开本地文件
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 0;
    }

    // 计算文件 SHA1
    char sha1Hex[41] = {0};
    if (sha1_fd(fd, sha1Hex) != 0) {
        perror("sha1");
        close(fd);
        return -1;
    }

    // 获取文件大小
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return -1;
    }
    off_t filesize = st.st_size;

    // 构造并发送 puts 命令
    memset(command, 0, sizeof(command_t));
    command->cmd = CMD_PUTS;
    strcpy(command->data, filename);
    command->len = strlen(command->data);

    if (sendCommand(sfd, command) == -1) {
        perror("sendCommand");
        close(fd);
        return -1;
    }

    // 发送 SHA1，让服务端判断是否秒传
    if (sendResponse(sfd, sha1Hex) == -1) {
        perror("sendResponse sha1");
        close(fd);
        return -1;
    }

    // 等待服务端决定：SKIP(秒传) 或 READY(需要上传)
    char response[100] = {0};
    if (recvResponse(sfd, response, sizeof(response)) <= 0) {
        perror("recvResponse");
        close(fd);
        return -1;
    }

    if (strcmp(response, "SKIP") == 0) {
        printf("秒传成功: %s\n", filename);
        close(fd);
        return 0;
    }

    if (strcmp(response, "READY") != 0) {
        printf("Server refused: %s\n", response);
        close(fd);
        return 0;
    }

    // 发送文件大小
    if (sendAll(sfd, &filesize, sizeof(filesize)) == -1) {
        perror("sendAll filesize");
        close(fd);
        return -1;
    }

    // 发送文件内容（sha1_fd 已把偏移复位到 0）
    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (sendAll(sfd, buf, n) == -1) {
            perror("sendAll file");
            close(fd);
            return -1;
        }
    }
    close(fd);

    // 等待服务器最终回复
    memset(response, 0, sizeof(response));
    if (recvResponse(sfd, response, sizeof(response)) <= 0) {
        perror("recvResponse");
        return -1;
    }
    if (strcmp(response, "OK") == 0) {
        printf("Upload success\n");
    } else {
        printf("Upload failed: %s\n", response);
    }
    return 0;
}

int removeFunction(int sfd, command_t *command, char *argv[], int argc)
{
    if (argc != 2) {
        printf("Format error\n");
        return -1;
    }

    memset(command, 0, sizeof(command_t));

    command->cmd = CMD_REMOVE;

    strcpy(command->data, argv[1]);
    command->len = strlen(command->data);

    if (sendCommand(sfd, command) == -1) {
        perror("sendCommand");
        return -1;
    }

    char response[1000] = {0};

    if (recvResponse(sfd, response, sizeof(response)) <= 0) {
        perror("recvResponse");
        return -1;
    }

    if (strcmp(response, "OK") == 0) {
        printf("Remove success\n");
        return 0;
    }

    printf("This file does not exist\n");
    return -1;
}
