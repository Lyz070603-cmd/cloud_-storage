#include "function.h"
#include "initTcp.h"
#include "epollsimple.h"
#include "login.h"
#include "logrecord.h"
#include "protocol.h"
#include "encodingtoken.h"
#include "datathread.h"
#include "idleTimer.h"

#define MAX_CLIENTS 1024

int exitpipe[2] = {0};

// 30 秒不活跃踢人环形队列（仅主线程访问，无需加锁）
static idleTimer_t idleTimer;

void handler(int signum){
    write(exitpipe[1], "1", sizeof(char));
}

// 短命令分发（主线程直接处理，命令已经读取，不再二次读取）
static int dispatchShortCommand(clientInfo_t *client, command_t *command){
    int ret;
    switch (command->cmd) {
        case CMD_CD:     ret = handleCd(client, command->data);     break;
        case CMD_LS:     ret = handleLs(client, command->data);     break;
        case CMD_PWD:    ret = handlePwd(client);                   break;
        case CMD_REMOVE: ret = handleRemove(client, command->data); break;
        default:
            sendResponse(client->netfd, "Unknown command");
            return -1;
    }
    (void)ret;
    return 0;
}

// 移除一个客户端：从 epoll 摘除、关库、关连接、释放、数组移除
static void removeClient(clientInfo_t **arr, int *num, int epfd, clientInfo_t *ci){
    epollDelet(epfd, ci->netfd);
    mysql_close(ci->mysql);
    close(ci->netfd);
    for (int j = 0; j < *num; ++j) {
        if (arr[j] == ci) {
            arr[j] = arr[*num - 1];
            (*num)--;
            break;
        }
    }
    idleTimerRemove(&idleTimer, ci);  // 从环形队列摘除，避免悬空
    free(ci);
}

// 计时器心跳：按实际经过的整秒推进环形队列，踢出 30 秒不活跃的用户
static void tickIdleTimer(clientInfo_t **arr, int *num, int epfd, struct timespec *lastTick){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long long elapsedMs = (now.tv_sec - lastTick->tv_sec) * 1000LL
                        + (now.tv_nsec - lastTick->tv_nsec) / 1000000LL;
    int ticks = (int)(elapsedMs / 1000);
    if (ticks <= 0) return;

    // 只推进整秒，不足 1 秒的余量保留到下一轮
    lastTick->tv_sec += ticks;

    // 推进一圈即可：一圈后所有格子都已被清空，多出来的秒数无意义
    if (ticks > IDLE_SLOTS) ticks = IDLE_SLOTS;

    clientInfo_t *expired[MAX_CLIENTS];
    for (int i = 0; i < ticks; ++i){
        int n = idleTimerAdvance(&idleTimer, expired, MAX_CLIENTS);
        for (int k = 0; k < n; ++k){
            removeClient(arr, num, epfd, expired[k]);
        }
    }
}

int main(int argc, char *argv[]){
    ARGS_CHECK(argc, 4);
    // argv[1]=ip, argv[2]=主端口, argv[3]=上传下载线程数

    pipe(exitpipe);
    if (fork() != 0){
        signal(SIGUSR1, handler);
        wait(NULL);
        return 0;
    }

    int sfd = initTcp(argv[1], argv[2]);
    mkdir("test", 0777);
    LOG_INIT();

    int epfd = epoll_create(1);
    epollAdd(epfd, sfd);
    epollAdd(epfd, exitpipe[0]);

    int mainPort = atoi(argv[2]);
    int threadNum = atoi(argv[3]);
    if (threadNum > MAX_DATA_THREADS) threadNum = MAX_DATA_THREADS;

    // 启动上传/下载线程，每个监听一个独立数据端口
    dataThreadArg_t dataArgs[MAX_DATA_THREADS];
    pthread_t dataTids[MAX_DATA_THREADS];
    for (int i = 0; i < threadNum; ++i) {
        strncpy(dataArgs[i].ip, argv[1], sizeof(dataArgs[i].ip) - 1);
        dataArgs[i].ip[sizeof(dataArgs[i].ip) - 1] = '\0';
        dataArgs[i].port = mainPort + 1 + i;
        dataArgs[i].index = i;
        pthread_create(&dataTids[i], NULL, dataThreadFunc, &dataArgs[i]);
    }

    // 客户端集合
    clientInfo_t *clientArr[MAX_CLIENTS];
    int clientnum = 0;

    srand((unsigned int)(time(NULL) ^ getpid()));

    // 初始化 30 秒不活跃环形队列与心跳基准时间
    idleTimerInit(&idleTimer);
    struct timespec lastTick;
    clock_gettime(CLOCK_MONOTONIC, &lastTick);

    while (1){
        struct epoll_event readyEvent[1024];
        int readyNum = epoll_wait(epfd, readyEvent, 1024, 1000);  // 1 秒超时，供心跳计时

        for (int i = 0; i < readyNum; ++i){
            int fd = readyEvent[i].data.fd;

            if (fd == sfd){
                // 新连接
                int netfd = accept(sfd, NULL, NULL);
                if (netfd == -1) continue;

                // 为客户端建立数据库连接
                MYSQL *mysql = mysql_init(NULL);
                if (mysql == NULL ||
                    mysql_real_connect(mysql, "localhost", "root", "Lyz610831", "mydb", 0, NULL, 0) == NULL){
                    fprintf(stderr, "mysql connect failed\n");
                    if (mysql) mysql_close(mysql);
                    close(netfd);
                    continue;
                }

                clientInfo_t *ci = (clientInfo_t *)calloc(1, sizeof(clientInfo_t));
                ci->netfd = netfd;
                ci->mysql = mysql;

                if (loginhandle(ci) == 0){
                    clientArr[clientnum++] = ci;
                    epollAdd(epfd, netfd);
                    idleTimerTouch(&idleTimer, ci);  // 登录成功即入队，开始计时
                    printf("accept new client\n");
                } else {
                    mysql_close(mysql);
                    close(netfd);
                    free(ci);
                }
            }
            else if (fd == exitpipe[0]){
                // 有序退出
                for (int j = 0; j < threadNum; ++j) pthread_join(dataTids[j], NULL);
                return 0;
            }
            else {
                // 已登录客户端的命令
                clientInfo_t *ci = NULL;
                for (int j = 0; j < clientnum; ++j){
                    if (clientArr[j]->netfd == fd){ ci = clientArr[j]; break; }
                }
                if (ci == NULL) continue;

                command_t command;
                int ret = recvCommand(fd, &command);
                if (ret <= 0){
                    removeClient(clientArr, &clientnum, epfd, ci);
                    continue;
                }

                // 用户活跃（收到命令），重置 30 秒计时：删除旧记录 + 放入最新格子
                idleTimerTouch(&idleTimer, ci);

                if (command.cmd == CMD_PUTS || command.cmd == CMD_GETS){
                    // 长命令：生成 token + 随机数据端口 + 当前路径
                    char token[TOKEN_BUF_SIZE] = {0};
                    createToken(ci->name, token, sizeof(token));

                    int dataPort = mainPort + 1 + (rand() % threadNum);

                    char path[1024] = {0};
                    getCwdPath(ci, path, sizeof(path));

                    sendResponse(fd, token);                    // 1. token
                    sendAll(fd, &dataPort, sizeof(dataPort));   // 2. 端口
                    sendResponse(fd, path);                     // 3. 当前路径
                } else {
                    // 短命令：主线程处理
                    dispatchShortCommand(ci, &command);
                }
            }
        }

        // 心跳：推进环形队列，踢出 30 秒不活跃的用户
        tickIdleTimer(clientArr, &clientnum, epfd, &lastTick);
    }
    return 0;
}
