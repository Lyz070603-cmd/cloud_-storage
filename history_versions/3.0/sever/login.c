#include "login.h"
#include <crypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int loginhandle(clientInfo_t *clientInfo){
    while(1){
        int flag = 0;
        int ret = recvAll(clientInfo->netfd, &flag, sizeof(flag));
        if (ret <= 0) {
            return -1;
        }
        //注册账户
        if (flag == SIGNUP){

            while (1) {

                char name[64] = {0};
                char passwd[32] = {0};

                // 接收用户名
                ret = recvResponse(clientInfo->netfd,
                                   name,
                                   sizeof(name));

                if (ret <= 0) {
                    return -1;
                }

                // 接收密码
                ret = recvResponse(clientInfo->netfd,
                                   passwd,
                                   sizeof(passwd));

                if (ret <= 0) {
                    return -1;
                }

                printf("Register username: %s\n", name);

                // 转义用户名，防止 SQL 注入
                char nameEscaped[2 * sizeof(name) + 1] = {0};
                mysql_real_escape_string(clientInfo->mysql,
                                         nameEscaped,
                                         name,
                                         (unsigned long)strlen(name));

                // 查询用户名是否已经存在
                char sql[512] = {0};

                snprintf(sql,
                         sizeof(sql),
                         "SELECT id FROM User WHERE name='%s'",
                         nameEscaped);

                if (mysql_query(clientInfo->mysql, sql) != 0) {
                    sendResponse(clientInfo->netfd, "ERROR");
                    continue;
                }

                // 获取查询结果
                MYSQL_RES *result = mysql_store_result(clientInfo->mysql);

                if (result == NULL) {
                    sendResponse(clientInfo->netfd, "ERROR");
                    continue;
                }

                // 用户名已经存在
                if (mysql_num_rows(result) > 0) {
                    mysql_free_result(result);
                    sendResponse(clientInfo->netfd,
                                 "USER_EXISTS");
                    continue;
                }

                mysql_free_result(result);
                // 生成随机 salt
                char salt[64] = {0};
                if (generateSalt(salt) == -1) {

                    sendResponse(clientInfo->netfd,
                                 "ERROR");

                    continue;
                }

                // 使用 crypt 对密码进行加密
                char *ciphertext = crypt(passwd, salt);
                if (ciphertext == NULL) {

                    perror("crypt");

                    sendResponse(clientInfo->netfd,
                                 "ERROR");

                    continue;
                }
                // 将用户名作为根目录行写入数据库（虚拟目录的根）
                memset(sql, 0, sizeof(sql));
                snprintf(sql,
                         sizeof(sql),
                         "INSERT INTO User(name,filename,pre_id,path,type,sha1,tomb,password,salt) "
                         "VALUES('%s',NULL,NULL,'/','dir',NULL,0,'%s','%s')",
                         nameEscaped,
                         ciphertext,
                         salt);

                if (mysql_query(clientInfo->mysql, sql) != 0) {
                    sendResponse(clientInfo->netfd,
                                 "ERROR");
                    continue;
                }

                // 注册成功
                printf("Register success: %s\n", name);
                sendResponse(clientInfo->netfd, "OK");

                break;
            }    
        }
        //登录账户
        else if (flag == LOGIN){

            while (1) {

                char name[64] = {0};
                char salt[64] = {0};
                char databasePassword[256] = {0};
                char clientPassword[256] = {0};

                // 接收用户名
                ret = recvResponse(clientInfo->netfd,
                                   name,
                                   sizeof(name));
                if (ret <= 0) {
                    return -1;
                }
                // 转义用户名，防止 SQL 注入
                char nameEscaped[2 * sizeof(name) + 1] = {0};
                mysql_real_escape_string(clientInfo->mysql,
                                         nameEscaped,
                                         name,
                                         (unsigned long)strlen(name));

                // 查询用户根目录行（id、salt、密码密文）
                char sql[512] = {0};

                snprintf(sql,
                         sizeof(sql),
                         "SELECT id,salt,password FROM User WHERE name='%s' AND filename IS NULL",
                         nameEscaped);

                if (mysql_query(clientInfo->mysql, sql) != 0) {
                    sendResponse(clientInfo->netfd, "ERROR");
                    continue;
                }
                // 获取查询结果
                MYSQL_RES *result = mysql_store_result(clientInfo->mysql);
                if (result == NULL) {
                    sendResponse(clientInfo->netfd, "ERROR");
                    continue;
                }

                // 用户名不存在
                if (mysql_num_rows(result) == 0) {
                    mysql_free_result(result);
                    sendResponse(clientInfo->netfd,
                                 "USER_NOT_FOUND");

                    continue;
                }

                // 获取查询结果
                MYSQL_ROW row = mysql_fetch_row(result);

                if (row == NULL) {
                    mysql_free_result(result);
                    sendResponse(clientInfo->netfd,
                                 "ERROR");
                    continue;
                }
                // 保存用户根目录行 id
                int rootId = atoi(row[0]);

                // 保存数据库中的 salt
                strncpy(salt, row[1], sizeof(salt) - 1);
                salt[sizeof(salt) - 1] = '\0';

                // 保存数据库中的密码密文
                strncpy(databasePassword, row[2], sizeof(databasePassword) - 1);
                databasePassword[sizeof(databasePassword) - 1] = '\0';

                mysql_free_result(result);

                // 将 salt 发送给客户端
                if (sendResponse(clientInfo->netfd, salt) == -1) {
                    return -1;
                }

                // 接收客户端使用 salt 加密后的密码
                ret = recvResponse(clientInfo->netfd,
                                   clientPassword,
                                   sizeof(clientPassword));

                if (ret <= 0) {
                    return -1;
                }

                // 比较客户端密文和数据库密文
                if (strcmp(clientPassword, databasePassword) != 0) {
                    sendResponse(clientInfo->netfd,
                                 "PASSWORD_ERROR");

                    continue;
                }

                // 登录成功
                printf("Login success: %s\n", name);
                strncpy(clientInfo->name, name, sizeof(clientInfo->name) - 1);
                clientInfo->name[sizeof(clientInfo->name) - 1] = '\0';

                // 初始化虚拟目录：根目录 id、当前目录 id、空路径栈
                clientInfo->rootId = rootId;
                clientInfo->curDirId = rootId;
                clientInfo->cwdTop = 0;

                sendResponse(clientInfo->netfd, "OK");
                return 0;
            }
        }
    }
    return 0;
}


int generateSalt(char *salt)
{
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)(time(NULL) ^ getpid()));
        seeded = 1;
    }

    const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";

    int len = sizeof(charset) - 1;

    strcpy(salt, "$6$");

    for (int i = 0; i < SALT_LEN; ++i) {
        salt[3 + i] = charset[rand() % len];
    }

    salt[3 + SALT_LEN] = '$';
    salt[4 + SALT_LEN] = '\0';

    return 0;
}
