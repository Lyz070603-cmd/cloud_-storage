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

            case CMD_CD:{
                ret = handleCd(client, command.data);
                char st[128] = {0};
                sprintf(st,"cd %s",command.data);
                LOG(client->name,st,ret);
                break;
            }
            case CMD_LS:{
                ret = handleLs(client,command.data);
                LOG(client->name,"ls",ret);
                break;
            }
            case CMD_PWD:{
                ret = handlePwd(client);
                LOG(client->name,"pwd",ret);
                break;
            }
            case CMD_PUTS:{
                ret = handlePuts(client, command.data);
                char st[128] = {0};
                sprintf(st,"puts %s",command.data);
                LOG(client->name,st,ret);
                break;
            }
            case CMD_GETS:{
                ret = handleGets(client, command.data);
                char st[128] = {0};
                sprintf(st,"gets %s",command.data);
                LOG(client->name,st,ret);
                break;
            }
            case CMD_REMOVE:{
                ret = handleRemove(client, command.data);
                char st[128] = {0};
                sprintf(st,"remove %s",command.data);
                LOG(client->name,st,ret);
                break;
            }
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
