#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <sys/types.h>


/*
 * 客户端命令类型
 */
#define CMD_RESPONSE 0
#define CMD_CD       1
#define CMD_LS       2
#define CMD_PWD      3
#define CMD_PUTS     4
#define CMD_GETS     5
#define CMD_REMOVE   6


/*
 * 普通命令协议
 */
typedef struct {
    int cmd;
    int len;
    char data[1000];
} command_t;


//保证发送完整数据
int sendAll(int fd, const void *buf, size_t len);
// 保证接收完整数据
// 返回：
//  1  成功
//  0  对端关闭
// -1  出错
int recvAll(int fd, void *buf, size_t len);
//发送一个完整命令
int sendCommand(int fd, const command_t *command);
//接收一个完整命令
int recvCommand(int fd, command_t *command);
//发送普通响应
int sendResponse(int fd, const char *response);
// 接收普通响应
int recvResponse(int fd, char *buf, size_t bufSize);

#endif
