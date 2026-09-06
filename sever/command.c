#include "command.h"
#include "protocol.h"

int handleCd(clientInfo_t *client, const char *path)
{
    char realPath[2048] = {0};
    char newCwd[1024] = {0};
    //cd /
    if (strcmp(path,"/") == 0){
        strcpy(newCwd,"/");

    }
    //cd .
    else if (strcmp(path,".") == 0){
        strcpy(newCwd,client->cwd);    
    }
    //cd ..
    else if (strcmp(path,"..") == 0){
    strcpy(newCwd,client->cwd);
        if (strcmp(client->cwd,"/") == 0){
            //已经是最后一个了，无法再后退
            sendResponse(client->netfd, "Already at root\n");
            return 0;
        }
        else{
            char *p = strrchr(newCwd,'/');  
            //判断是否是最后一个/
            if (strcmp(p,newCwd) == 0){
                //说明此时回退到/目录，只需要把根目录后面改掉就可以
                newCwd[1] = '\0';
            }
            else{
                *p = '\0';
            }

        }
    }
    //cd 正常
    else {
        
        if (path[0] == '/') {
            // 暂时不允许 cd /xxx
            // 后面可以扩展
            snprintf(newCwd,sizeof(newCwd),"%s",path);
        }
        else {

            if (strcmp(client->cwd, "/") == 0) {

                snprintf(newCwd,sizeof(newCwd),"/%s",path);
            }
            else {
                snprintf(newCwd,sizeof(newCwd),"%s/%s",
                         client->cwd,
                         path);
            }
        }
    }

    snprintf(realPath,sizeof(realPath),"%s%s",client->home,newCwd);
    // 判断目录是否存在
    struct stat st;
    if (stat(realPath, &st) == -1) {

        sendResponse(client->netfd,
                     "Directory does not exist\n");

        return -1;
    }
    //判断是不是目录
     if (!S_ISDIR(st.st_mode)) {

        sendResponse(client->netfd,
                     "Not a directory\n");

        return -1;
    }
    //开始真正的修改
     strcpy(client->cwd, newCwd);
     //返回客户端消息
     sendResponse(client->netfd, "OK\n");
    return 0;
}

int handleLs(clientInfo_t *client)
{
    return 0;
}

int handlePwd(clientInfo_t *client)
{   
    char ccwd[1024] = {0};
    sprintf(ccwd,"%s\n",client->cwd);
    sendResponse(client->netfd,ccwd);

    return 0;
}

int handlePuts(clientInfo_t *client, const char *filename)
{
    return 0;
}

int handleGets(clientInfo_t *client, const char *filename)
{
    return 0;
}

int handleRemove(clientInfo_t *client, const char *filename)
{
    return 0;
}
