#include "threadworkflow.h"
#include <stdlib.h>
#include <unistd.h>

int thread_workflow(clientInfo_t * clientInfo){
    //登录和注册函数
    if(loginhandle(clientInfo) != 0){
        // 登录/注册失败或对端断开：关闭连接并释放资源
        close(clientInfo->netfd);
        free(clientInfo);
        return 0;
    }

    workFunc(clientInfo);   // workFunc 内部会 close(clientInfo->netfd)

    free(clientInfo);
    return 0;
}
