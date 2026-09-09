#include "login.h"

int loginhandle(clientInfo_t *clientInfo){

    char buf[1024] = {0};
    char res[64] = {0};
    char name[1024] = {0};
    int ret = 0;
    struct spwd *sp;

    while(1){
        //判断账户名称
        memset(buf,0,sizeof(buf));
        ret =  recvResponse(clientInfo->netfd,buf,sizeof(buf));
        if (ret == 0 || ret == -1){
            return 0;
        }
        //开始寻找用户名称

        if ((sp = getspnam(buf)) != NULL){
            strcpy(res,"OK");
            sendResponse(clientInfo->netfd,res);
            strcpy(name,buf);
            //开始接受密码
            while(1){
                memset(buf,0,sizeof(buf));
                recvResponse(clientInfo->netfd,buf,sizeof(buf));
                char salt[512] = {0}; 
                get_salt(salt,sp->sp_pwdp);
                
                char *encrypted = crypt(buf, salt);

                if(strcmp(sp->sp_pwdp,encrypted) == 0){

                    char home[1024] = {0};
                    sprintf(home,"%s/%s","/home/shangshansaye/sever_project/sever/test",name);
                    strcpy(clientInfo->home,home);
                    strcpy(clientInfo->cwd,"/");
                    strcpy(clientInfo->name,name);
                    //登录成功
                    //开始接受客户端的命令
                    memset(res,0,sizeof(res));
                    strcpy(res,"OK");
                    sendResponse(clientInfo->netfd,res);
                    workFunc(clientInfo);

                    return 0;
                }
                else {

                    memset(res,0,sizeof(res));
                    strcpy(res,"NO");
                    sendResponse(clientInfo->netfd,res);
                    continue;
                }


            }
        }        
        else {
            memset(res,0,sizeof(res));
            strcpy(res,"NO");
            sendResponse(clientInfo->netfd,res);
            continue;
        }



    }
    return 0;
}
void get_salt(char *salt,char *passwd)
{
    int i,j;
    //取出salt,i记录密码字符下标,j记录$出现次数
    for(i=0,j=0;passwd[i] && j != 4;++i)
    {
        if(passwd[i] == '$')
            ++j;
    }
    strncpy(salt,passwd,i);
}
