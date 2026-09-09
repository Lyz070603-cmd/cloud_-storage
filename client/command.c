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
    if (argc != 2) {
        printf("Format error\n");
        return 0;
    }

    const char *filename = argv[1];

    // 检查本地是否存在该文件
    int fd = open(filename, O_RDWR);

    off_t offset = 0;

    if (fd == -1) {

        // 文件不存在，从头开始下载
        if (errno == ENOENT) {

            fd = open(filename,
                      O_RDWR | O_CREAT,
                      0666);

            if (fd == -1) {
                perror("open");
                return -1;
            }

            offset = 0;

        } else {

            perror("open");
            return -1;
        }

    } else {

        // 文件存在，获取本地文件大小
        struct stat st;

        if (fstat(fd, &st) == -1) {
            perror("fstat");
            close(fd);
            return -1;
        }

        offset = st.st_size;

        printf("Local file exists, resume from %ld bytes\n", offset);
    }

    // 构造 gets 命令
    memset(command, 0, sizeof(command_t));

    command->cmd = CMD_GETS;
    strcpy(command->data, filename);
    command->len = strlen(command->data);

    // 发送 gets 命令
    if (sendCommand(sfd, command) == -1) {
        perror("sendCommand");
        close(fd);
        return -1;
    }

    // 等待服务器回复 READY
    char response[1000] = {0};

    if (recvResponse(sfd, response, sizeof(response)) == -1) {
        perror("recvResponse");
        close(fd);
        return -1;
    }

    // 判断服务器是否允许下载
    if (strcmp(response, "READY") != 0) {
        printf("Server response: %s\n", response);
        close(fd);
        return 0;
    }

    // 将本地已经拥有的文件大小发送给服务器
    if (sendAll(sfd, &offset, sizeof(offset)) == -1) {
        perror("sendAll offset");
        close(fd);
        return -1;
    }

    // 接收服务器文件总大小
    off_t filesize = 0;

    if (recvAll(sfd, &filesize, sizeof(filesize)) == -1) {
        perror("recvAll filesize");
        close(fd);
        return -1;
    }

    printf("Server filesize = %ld\n", filesize);

    // 如果本地文件比服务器文件大，则重新下载
    if (offset > filesize) {

        printf("Local file is larger than server file, restart download\n");

        if (ftruncate(fd, 0) == -1) {
            perror("ftruncate");
            close(fd);
            return -1;
        }

        offset = 0;
    }

    // 如果本地文件已经完整下载
    if (offset == filesize) {

        printf("File already complete\n");

        close(fd);

        // 接收服务器最终回复
        memset(response, 0, sizeof(response));

        if (recvResponse(sfd, response, sizeof(response)) == -1) {
            perror("recvResponse");
            return -1;
        }

        if (strcmp(response, "OK") == 0) {
            printf("Download success\n");
        }

        return 0;
    }

    // 定位到断点位置
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        close(fd);
        return -1;
    }

    // 接收服务器剩余数据
    off_t received = offset;
    char buf[1024];

    while (received < filesize) {

        // 计算本次需要接收的数据大小
        size_t recvSize = sizeof(buf);

        if (filesize - received < (off_t)recvSize) {
            recvSize = filesize - received;
        }

        // 接收文件数据
        int ret = recvAll(sfd, buf, recvSize);

        if (ret == -1) {
            perror("recvAll file");
            close(fd);
            return -1;
        }

        // 将数据写入本地文件
        if (write(fd, buf, recvSize) != (ssize_t)recvSize) {
            perror("write");
            close(fd);
            return -1;
        }

        received += recvSize;

        // 显示下载进度
        double percent = (double)received / filesize * 100;

        printf("\rDownload: %.2f%%", percent);
        fflush(stdout);
    }

    printf("\n");

    close(fd);

    // 接收服务器最终回复
    memset(response, 0, sizeof(response));

    if (recvResponse(sfd, response, sizeof(response)) == -1) {
        perror("recvResponse");
        return -1;
    }

    // 判断下载是否成功
    if (strcmp(response, "OK") == 0) {
        printf("Download success\n");
    } else {
        printf("Download failed: %s\n", response);
    }

    return 0;
}

//客户端想要上传文件给服务端
int putsFunction(int sfd, command_t *command, char *argv[], int argc)
{
    if (argc != 2) {
        printf("Format error\n");
        return 0;
    }

    // 打开本地文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 0;
    }

    // 获取文件信息
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 0;
    }

    off_t filesize = st.st_size;

    // 构造 puts 命令
    memset(command, 0, sizeof(command_t));
    command->cmd = CMD_PUTS;
    strcpy(command->data, argv[1]);
    command->len = strlen(command->data);

    // 发送命令
    if (sendCommand(sfd, command) == -1) {
        perror("sendCommand");
        close(fd);
        return -1;
    }

    // 等待服务器 READY
    char response[1000] = {0};

    if (recvResponse(sfd, response, sizeof(response)) == -1) {
        perror("recvResponse");
        close(fd);
        return -1;
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

    /*
     * 大文件使用 mmap
     * 小文件使用 read
     */
    if (filesize > 100 * 1024 * 1024) {

        char *map = (char *)mmap(NULL,filesize,PROT_READ,MAP_PRIVATE,fd,0);

        if (map == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return -1;
        }

        off_t offset = 0;

        while (offset < filesize) {

            size_t sendSize = 1024 * 1024;   // 每次发送 1MB

            if (filesize - offset < (off_t)sendSize) {
                sendSize = filesize - offset;
            }

            if (sendAll(sfd,
                        map + offset,
                        sendSize) == -1) {

                perror("sendAll file");

                munmap(map, filesize);
                close(fd);
                return -1;
            }

            offset += sendSize;
        }

        munmap(map, filesize);

    } else {

        char buf[1024];

        while (1) {

            ssize_t ret = read(fd, buf, sizeof(buf));

            if (ret == 0) {
                break;
            }

            if (ret == -1) {
                perror("read");
                close(fd);
                return -1;
            }

            if (sendAll(sfd, buf, ret) == -1) {
                perror("sendAll file");
                close(fd);
                return -1;
            }
        }
    }

    close(fd);

    // 等待服务器最终回复
    memset(response, 0, sizeof(response));

    if (recvResponse(sfd, response, sizeof(response)) == -1) {
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
