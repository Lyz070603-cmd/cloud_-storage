#include "login.h"

int loginFunction(int sfd){
    char buf[1024] = {0};
    char res[64] = {0};
    int ret = 0;
    while(1){
        //第一次判断账户名称
        memset(buf,0,sizeof(buf));
        printf("Account Name:");
        read(STDIN_FILENO,buf,sizeof(buf));

        sendResponse(sfd,buf);

        memset(res,0,sizeof(res));
        ret = recvResponse(sfd,res,sizeof(res));
        if (ret == 0){
            printf("Server access denied\n");
            return -1;
        }
        //第二次判断账户密码
        if (strcmp(res,"OK") == 0){

            while(1){
                memset(buf,0,sizeof(buf));
                printf("Account Password:");
                read(STDIN_FILENO,buf,sizeof(buf));

                sendResponse(sfd,buf);

                memset(res,0,sizeof(res));
                recvResponse(sfd,res,sizeof(res));

                //登录成功返回0
                if (strcmp(res,"OK") == 0){
                    printf("Login successful\n");
                    return 0;
                }
                else{
                    printf("Account Password ERROR. Please enter again\n");
                    continue;
                }

            }
        }
        else{
            printf("Account Name ERROR. Please enter again\n");
            continue;
        }

    }

    return 0;
}
