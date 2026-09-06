#include "workFunction.h"

#ifndef __WORKFUNCTION__
#define __WORKFUNCTION__

//工作模式是循环读取
//然后处理读取的信息
//反馈给用户

int workFunc(clientInfo_t *client)
{
    while (1) {
        command_t command;
        // 1. 接收客户端发送的命令
        int ret = recvCommand(client->netfd, &command);
        if (ret == 0) {
            // 客户端断开
            break;
        }
        if (ret == -1) {
            // 接收出错
            perror("recvCommand");
            break;
        }
        // 2. 根据命令类型进行分发
        switch (command.cmd) {

            case CMD_CD:
                handleCd(client, command.data);
                break;

            case CMD_LS:
                handleLs(client);
                break;

            case CMD_PWD:
                handlePwd(client);
                break;

            case CMD_PUTS:
                handlePuts(client, command.data);
                break;

            case CMD_GETS:
                handleGets(client, command.data);
                break;

            case CMD_REMOVE:
                handleRemove(client, command.data);
                break;

            default:
                // 未知命令
                sendResponse(client->netfd, "Unknown command");
                break;
        }
    }

    close(client->netfd);
    return 0;
}


#endif
