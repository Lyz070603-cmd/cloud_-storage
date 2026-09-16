#include "datathread.h"
#include "function.h"
#include "protocol.h"
#include "command.h"
#include "encodingtoken.h"

// 建立数据端口监听，失败返回 -1（不退出，避免线程里调用 exit）
static int initDataListen(const char *ip, int port) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("socket");
        return -1;
    }
    int reuse = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sfd);
        return -1;
    }
    if (listen(sfd, 128) == -1) {
        perror("listen");
        close(sfd);
        return -1;
    }
    return sfd;
}

// 处理一次上传/下载数据连接
static void handleDataConnection(int cfd) {
    // 1. 用户名
    char name[64] = {0};
    if (recvResponse(cfd, name, sizeof(name)) <= 0) return;

    // 2. token
    char token[TOKEN_BUF_SIZE] = {0};
    if (recvResponse(cfd, token, sizeof(token)) <= 0) return;

    // 3. 校验 token
    if (verifyToken(name, token) != 0) {
        sendResponse(cfd, "TOKEN_ERROR");
        return;
    }

    // 4. 命令（puts/gets + 文件名）
    command_t command;
    if (recvCommand(cfd, &command) <= 0) return;

    // 5. 当前虚拟路径
    char path[1024] = {0};
    if (recvResponse(cfd, path, sizeof(path)) <= 0) return;

    // 6. 数据库连接
    MYSQL *mysql = mysql_init(NULL);
    if (mysql == NULL ||
        mysql_real_connect(mysql, "localhost", "root", "Lyz610831", "mydb", 0, NULL, 0) == NULL) {
        fprintf(stderr, "data thread mysql connect failed\n");
        sendResponse(cfd, "ERROR");
        if (mysql) mysql_close(mysql);
        return;
    }

    // 7. 重建目录状态
    clientInfo_t ci;
    memset(&ci, 0, sizeof(ci));
    ci.netfd = cfd;
    ci.mysql = mysql;
    strncpy(ci.name, name, sizeof(ci.name) - 1);
    ci.name[sizeof(ci.name) - 1] = '\0';

    char esc[2 * sizeof(name) + 1];
    mysql_real_escape_string(mysql, esc, name, (unsigned long)strlen(name));
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id FROM User WHERE name='%s' AND filename IS NULL", esc);

    int rootId = 0;
    if (mysql_query(mysql, sql) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (res != NULL) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != NULL && row[0] != NULL) rootId = atoi(row[0]);
            mysql_free_result(res);
        }
    }
    ci.rootId = rootId;
    ci.curDirId = rootId;
    ci.cwdTop = 0;
    resolveAndCommit(&ci, path);

    // 8. 分发
    if (command.cmd == CMD_PUTS) {
        handlePuts(&ci, command.data);
    } else if (command.cmd == CMD_GETS) {
        handleGets(&ci, command.data);
    }

    mysql_close(mysql);
}

void *dataThreadFunc(void *arg) {
    dataThreadArg_t *a = (dataThreadArg_t*)arg;

    int lfd = initDataListen(a->ip, a->port);
    if (lfd == -1) {
        fprintf(stderr, "data thread %d listen failed\n", a->index);
        return NULL;
    }
    printf("data thread %d listening on port %d\n", a->index, a->port);

    while (1) {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd == -1) {
            perror("accept");
            continue;
        }
        handleDataConnection(cfd);
        close(cfd);
    }
    return NULL;
}
