#include"command.h"
#include <openssl/evp.h>

// 计算打开文件的 SHA1，输出 40 位十六进制（hexout 至少 41 字节）
// 计算完成后把文件偏移复位到 0，方便后续再读
static int sha1_fd(int fd, char *hexout) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) return -1;
    if (EVP_DigestInit_ex(ctx, EVP_sha1(), NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    unsigned char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        EVP_DigestUpdate(ctx, buf, (size_t)n);
    }
    if (n == -1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    for (unsigned int i = 0; i < hashLen; ++i) {
        sprintf(hexout + i * 2, "%02x", hash[i]);
    }
    hexout[hashLen * 2] = '\0';

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

    // 1. 构造并发送 gets 命令到主连接
    memset(command, 0, sizeof(command_t));
    command->cmd = CMD_GETS;
    strcpy(command->data, filename);
    command->len = strlen(command->data);

    if (sendCommand(sfd, command) == -1) {
        perror("sendCommand");
        return -1;
    }

    // 2. 接收 token + 数据端口 + 当前路径
    char token[TOKEN_BUF_SIZE] = {0};
    int port = 0;
    char path[1024] = {0};
    if (recvResponse(sfd, token, sizeof(token)) <= 0) return -1;
    if (recvAll(sfd, &port, sizeof(port)) <= 0) return -1;
    if (recvResponse(sfd, path, sizeof(path)) <= 0) return -1;

    // 3. 连接数据端口
    int dfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in daddr;
    memset(&daddr, 0, sizeof(daddr));
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(port);
    daddr.sin_addr.s_addr = inet_addr(g_serverIp);
    if (connect(dfd, (struct sockaddr*)&daddr, sizeof(daddr)) == -1) {
        perror("connect data");
        return -1;
    }

    // 4. 发送 username + token + command + path 给数据线程
    if (sendResponse(dfd, g_username) == -1 ||
        sendResponse(dfd, token) == -1 ||
        sendCommand(dfd, command) == -1 ||
        sendResponse(dfd, path) == -1) {
        close(dfd);
        return -1;
    }

    // 5. 在数据连接上做下载
    char response[100] = {0};
    if (recvResponse(dfd, response, sizeof(response)) <= 0) {
        perror("recvResponse");
        close(dfd);
        return -1;
    }
    if (strcmp(response, "READY") != 0) {
        printf("Server response: %s\n", response);
        close(dfd);
        return 0;
    }

    off_t filesize = 0;
    if (recvAll(dfd, &filesize, sizeof(filesize)) <= 0) {
        perror("recvAll filesize");
        close(dfd);
        return -1;
    }

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        close(dfd);
        return -1;
    }

    char buf[8192];
    off_t received = 0;
    while (received < filesize) {
        size_t n = sizeof(buf);
        if (filesize - received < (off_t)n) {
            n = (size_t)(filesize - received);
        }
        if (recvAll(dfd, buf, n) <= 0) {
            perror("recvAll file");
            close(fd);
            close(dfd);
            return -1;
        }
        if (write(fd, buf, n) != (ssize_t)n) {
            perror("write");
            close(fd);
            close(dfd);
            return -1;
        }
        received += (off_t)n;

        double percent = (double)received / filesize * 100;
        printf("\rDownload: %.2f%%", percent);
        fflush(stdout);
    }
    printf("\n");
    close(fd);

    memset(response, 0, sizeof(response));
    if (recvResponse(dfd, response, sizeof(response)) <= 0) {
        close(dfd);
        return -1;
    }
    if (strcmp(response, "OK") == 0) {
        printf("Download success\n");
    } else {
        printf("Download failed: %s\n", response);
    }
    close(dfd);
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

    // 打开本地文件并计算 SHA1 / 大小
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 0;
    }
    char sha1Hex[41] = {0};
    if (sha1_fd(fd, sha1Hex) != 0) {
        perror("sha1");
        close(fd);
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return -1;
    }
    off_t filesize = st.st_size;

    // 1. 构造并发送 puts 命令到主连接
    memset(command, 0, sizeof(command_t));
    command->cmd = CMD_PUTS;
    strcpy(command->data, filename);
    command->len = strlen(command->data);

    if (sendCommand(sfd, command) == -1) {
        perror("sendCommand");
        close(fd);
        return -1;
    }

    // 2. 接收 token + 数据端口 + 当前路径
    char token[TOKEN_BUF_SIZE] = {0};
    int port = 0;
    char path[1024] = {0};
    if (recvResponse(sfd, token, sizeof(token)) <= 0) { close(fd); return -1; }
    if (recvAll(sfd, &port, sizeof(port)) <= 0) { close(fd); return -1; }
    if (recvResponse(sfd, path, sizeof(path)) <= 0) { close(fd); return -1; }

    // 3. 连接数据端口
    int dfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in daddr;
    memset(&daddr, 0, sizeof(daddr));
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(port);
    daddr.sin_addr.s_addr = inet_addr(g_serverIp);
    if (connect(dfd, (struct sockaddr*)&daddr, sizeof(daddr)) == -1) {
        perror("connect data");
        close(fd);
        return -1;
    }

    // 4. 发送 username + token + command + path 给数据线程
    if (sendResponse(dfd, g_username) == -1 ||
        sendResponse(dfd, token) == -1 ||
        sendCommand(dfd, command) == -1 ||
        sendResponse(dfd, path) == -1) {
        close(dfd);
        close(fd);
        return -1;
    }

    // 5. 在数据连接上做上传（先发 SHA1 判断秒传）
    if (sendResponse(dfd, sha1Hex) == -1) {
        close(dfd);
        close(fd);
        return -1;
    }
    char response[100] = {0};
    if (recvResponse(dfd, response, sizeof(response)) <= 0) {
        close(dfd);
        close(fd);
        return -1;
    }
    if (strcmp(response, "SKIP") == 0) {
        printf("秒传成功: %s\n", filename);
        close(dfd);
        close(fd);
        return 0;
    }
    if (strcmp(response, "READY") != 0) {
        printf("Server refused: %s\n", response);
        close(dfd);
        close(fd);
        return 0;
    }

    // 发送文件大小
    if (sendAll(dfd, &filesize, sizeof(filesize)) == -1) {
        close(dfd);
        close(fd);
        return -1;
    }

    // 发送文件内容（sha1_fd 已把偏移复位到 0）
    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (sendAll(dfd, buf, n) == -1) {
            perror("sendAll file");
            close(dfd);
            close(fd);
            return -1;
        }
    }
    close(fd);

    // 等待最终回复
    memset(response, 0, sizeof(response));
    if (recvResponse(dfd, response, sizeof(response)) <= 0) {
        close(dfd);
        return -1;
    }
    if (strcmp(response, "OK") == 0) {
        printf("Upload success\n");
    } else {
        printf("Upload failed: %s\n", response);
    }
    close(dfd);
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
