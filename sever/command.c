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

int handleLs(clientInfo_t *client,const char *path)
{
    //流程 先进行三个分支喷段，最后再合法性检查，最后传输客户端
    char targetPath[1024] = {0};
    char realTargetPath[1024] = {0};
    //-------------判断------------------
    //访问本目录
    if (path == NULL || strlen(path) == 0 || strcmp(path,".") == 0){
        sprintf(targetPath,"%s%s",client->home,client->cwd);
    }
    //从跟目录开始访问
    else if (path[0] == '/'){
        sprintf(targetPath, "%s%s",client->home,path);
    }
    //正常访问
    else {
        sprintf(targetPath,"%s%s/%s",client->home,client->cwd,path);
    }


    //realpath规范路径并判断路径是否合法（不包含限制）
    if (realpath(targetPath,realTargetPath) == 0){
        sendResponse(client->netfd,"Directory does not exist\n");
        return 0;
    }
    //------------------检查------------------

    //检查客户访问是否逃出限制
    size_t rootLen = strlen(client->home);
    if (strncmp(realTargetPath,
                client->home,
                rootLen) != 0 ||
        //防止同名用户乱用 （eg:home/zhangsan home/zhangsan2)
        (realTargetPath[rootLen] != '\0' &&
         realTargetPath[rootLen] != '/')){
             sendResponse(client->netfd,"Permission denied\n");
             return 0;
         }
    //开始遍历目录
    DIR * dir = opendir(realTargetPath);

    if (dir == NULL) {
        sendResponse(client->netfd,"Not a directory\n");
        return -1;
    }
    char res[1024] = {0};

    struct dirent  *p = NULL;
    while((p = readdir(dir)) != NULL){
        strcat(res,p->d_name);
        strcat(res," ");

    }
    strcat(res,"\n");


    sendResponse(client->netfd,res);
    closedir(dir);

    return 0;
}

int handlePwd(clientInfo_t *client)
{   
    char ccwd[1024] = {0};
    sprintf(ccwd,"%s\n",client->cwd);
    sendResponse(client->netfd,ccwd);

    return 0;
}

//客户端想要上传到服务端的文件
int handlePuts(clientInfo_t *client, const char *filename)
{
    char path[1024] = {0};

    sprintf(path, "%s%s/%s",
            client->home,
            client->cwd,
            filename);

    // 1. 告诉客户端可以上传
    sendResponse(client->netfd, "READY");

    // 2. 接收文件大小
    off_t filesize;

    int ret = recvAll(client->netfd,
                      &filesize,
                      sizeof(filesize));

    if (ret <= 0) {
        return -1;
    }

    printf("filesize = %ld\n", filesize);

    // 3. 创建服务器文件
    int fd = open(path,
                  O_WRONLY | O_CREAT | O_TRUNC,
                  0666);

    if (fd == -1) {
        perror("open");
        return -1;
    }

    // 4. 接收文件内容
    char buf[1024];

    while (filesize > 0) {

        int recvSize = sizeof(buf);

        if (filesize < recvSize) {
            recvSize = filesize;
        }

        ret = recvAll(client->netfd,
                      buf,
                      recvSize);

        if (ret <= 0) {
            close(fd);
            return -1;
        }

        write(fd, buf, recvSize);
        filesize -= recvSize;
    }

    close(fd);

    // 5. 告诉客户端上传完成
    sendResponse(client->netfd, "OK");
    printf("Upload success\n");

    return 0;
}

//客户端想要从服务端获取的文件
int handleGets(clientInfo_t *client, const char *filename)
{
    char newpath[1024] = {0};

    sprintf(newpath,"%s%s/%s",client->home,client->cwd,filename);

    //printf("%s\n", newpath);

    // 打开服务器文件
    int fd = open(newpath, O_RDONLY);
    if (fd == -1) {
        sendResponse(client->netfd, "NOT_FOUND");
        return -1;
    }

    // 获取文件信息
    struct stat st;

    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        close(fd);
        sendResponse(client->netfd, "ERROR");
        return 0;
    }

    off_t filesize = st.st_size;

    // 告诉客户端：文件存在，可以开始传输
    if (sendResponse(client->netfd, "READY") == -1) {
        close(fd);
        return -1;
    }

    // 发送文件大小
    if (sendAll(client->netfd,&filesize,sizeof(filesize)) == -1) {
        close(fd);
        return -1;
    }

    // 开始发送文件
    char buf[1024];
    while (1) {
        ssize_t ret = read(fd, buf, sizeof(buf));
        if (ret == 0) {
            break;
        }

        if (sendAll(client->netfd,buf,ret) == -1) {
            close(fd);
            return -1;
        }
    }

    close(fd);

    // 文件传输完成
    sendResponse(client->netfd, "OK");
    return 0;
}

int handleRemove(clientInfo_t *client, const char *filename)
{
    return 0;
}
