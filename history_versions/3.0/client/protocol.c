#include "protocol.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


int wordSegmentation(char *buf,int *argc,char *argv[],command_t * command){
    char *p = strtok(buf, " \t\n");

    while (p != NULL && *argc < MAX_ARGS) {
        argv[*argc] = p;
        (*argc)++;
        p = strtok(NULL, " \t\n");
    }

    if (*argc == 0) {
        return 0 ;
    }

    if (strcmp(argv[0], "ls") == 0) {
        command->cmd = CMD_LS;
    }
    else if (strcmp(argv[0], "pwd") == 0) {
        command->cmd = CMD_PWD;

    }
    else if (strcmp(argv[0], "cd") == 0) {
        command->cmd = CMD_CD;

    }
    else if (strcmp(argv[0], "puts") == 0) {
        command->cmd = CMD_PUTS;

    }
    else if (strcmp(argv[0], "gets") == 0) {
        command->cmd = CMD_GETS;

    }
    else if (strcmp(argv[0], "remove") == 0) {
        command->cmd = CMD_REMOVE;

    }

    return 0;
}

int sendAll(int fd, const void *buf, size_t len)
{
    const char *p = (char *)buf;
    size_t total = 0;

    while (total < len) {

        ssize_t ret = send(fd,
                           p + total,
                           len - total,
                           MSG_NOSIGNAL);
        if (ret == -1) {

            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (ret == 0) {
            return -1;
        }
        total += ret;
    }
    return 0;
}
int recvAll(int fd, void *buf, size_t len)
{
    char *p = (char *)buf;
    size_t total = 0;

    while (total < len) {

        ssize_t ret = recv(fd,
                           p + total,
                           len - total,
                           0);

        if (ret == 0) {
            return 0;
        }

        if (ret == -1) {

            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        total += ret;
    }

    return 1;
}
int sendCommand(int fd, const command_t *command)
{
    if (command == NULL) {
        return -1;
    }

    if (command->len < 0 ||
        command->len >= (int)sizeof(command->data)) {
        return -1;
    }

    return sendAll(fd,
                   command,
                   sizeof(command_t));
}
int recvCommand(int fd, command_t *command)
{
    if (command == NULL) {
        return -1;
    }

    int ret = recvAll(fd,
                      command,
                      sizeof(command_t));

    if (ret <= 0) {
        return ret;
    }

    /*
     * 检查命令类型
     */
    if (command->cmd < CMD_RESPONSE ||
        command->cmd > CMD_REMOVE) {

        return -1;
    }

    /*
     * 检查数据长度
     */
    if (command->len < 0 ||
        command->len >= (int)sizeof(command->data)) {

        return -1;
    }

    /*
     * 确保字符串结尾
     */
    command->data[command->len] = '\0';

    return 1;
}
int sendResponse(int fd, const char *response)
{
    if (response == NULL) {
        return -1;
    }

    command_t command;

    memset(&command, 0, sizeof(command));

    command.cmd = CMD_RESPONSE;

    command.len = strlen(response);

    if (command.len >= (int)sizeof(command.data)) {
        return -1;
    }

    memcpy(command.data,
           response,
           command.len);

    return sendCommand(fd, &command);
}
int recvResponse(int fd, char *buf, size_t bufSize)
{
    if (buf == NULL || bufSize == 0) {
        return -1;
    }

    command_t command;

    int ret = recvCommand(fd, &command);

    if (ret <= 0) {
        return ret;
    }

    if (command.cmd != CMD_RESPONSE) {
        return -1;
    }

    if ((size_t)command.len + 1 > bufSize) {
        return -1;
    }

    memcpy(buf,
           command.data,
           command.len);

    buf[command.len] = '\0';

    return command.len;
}
