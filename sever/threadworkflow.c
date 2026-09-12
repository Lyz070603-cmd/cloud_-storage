#include "threadworkflow.h"

int thread_workflow(clientInfo_t * clientInfo){
    //登录和注册函数
    if(loginhandle(clientInfo) != 0){
        return 0;
    }

    workFunc(clientInfo);

    return 0;
}
