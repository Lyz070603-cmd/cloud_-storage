#include "login.h"
#include <crypt.h>


// 注册或登录
int loginFunction(int sfd)
{
    int flag = 0;

    while (1) {

        // 显示菜单
        printf("\n");
        printf("========== Cloud Storage ==========\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("0. Exit\n");
        printf("Please choose: ");
        fflush(stdout);

        // 获取用户选择
        char buf[16] = {0};
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            return -1;
        }
        int choice = atoi(buf);
        // 退出
        if (choice == 0) {
            return 0;
        }

        // 注册
        if (choice == 1) {
            flag = SIGNUP;
        }

        // 登录
        else if (choice == 2) {
            flag = LOGIN;
        }

        // 输入错误
        else {
            printf("Invalid choice, please try again\n");
            continue;
        }

        // 将注册或登录标志发送给服务器
        if (sendAll(sfd, &flag, sizeof(flag)) == -1) {
            perror("sendAll flag");
            return -1;
        }

        // 注册
        if (flag == SIGNUP) {

            while (1) {

                char name[64] = {0};
                char passwd[32] = {0};
                char response[100] = {0};

                // 输入用户名
                printf("Username: ");
                fflush(stdout);

                if (fgets(name, sizeof(name), stdin) == NULL) {
                    return -1;
                }

                name[strcspn(name, "\n")] = '\0';

                // 输入密码
                printf("Password: ");
                fflush(stdout);

                if (fgets(passwd, sizeof(passwd), stdin) == NULL) {
                    return -1;
                }

                passwd[strcspn(passwd, "\n")] = '\0';

                // 发送用户名
                if (sendResponse(sfd, name) == -1) {
                    perror("sendResponse name");
                    return -1;
                }

                // 发送密码
                if (sendResponse(sfd, passwd) == -1) {
                    perror("sendResponse passwd");
                    return -1;
                }

                // 接收服务器处理结果
                if (recvResponse(sfd,
                                 response,
                                 sizeof(response)) <= 0) {
                    perror("recvResponse");
                    return -1;
                }

                // 注册成功
                if (strcmp(response, "OK") == 0) {
                    printf("Register success\n");
                    break;
                }

                // 用户名已经存在
                if (strcmp(response, "USER_EXISTS") == 0) {
                    printf("Username already exists, please try again\n");
                    continue;
                }

                // 注册失败
                printf("Register failed, please try again\n");
            }
        }

        // 登录
        else if (flag == LOGIN) {

            while (1) {

                char name[64] = {0};
                char passwd[32] = {0};
                char salt[64] = {0};
                char response[100] = {0};

                // 输入用户名
                printf("Username: ");
                fflush(stdout);

                if (fgets(name, sizeof(name), stdin) == NULL) {
                    return -1;
                }

                name[strcspn(name, "\n")] = '\0';

                // 发送用户名
                if (sendResponse(sfd, name) == -1) {
                    perror("sendResponse name");
                    return -1;
                }

                // 接收服务器发送的 salt
                if (recvResponse(sfd,
                                 salt,
                                 sizeof(salt)) <= 0) {
                    perror("recvResponse salt");
                    return -1;
                }

                // 用户名不存在
                if (strcmp(salt, "USER_NOT_FOUND") == 0) {
                    printf("Username does not exist, please try again\n");
                    continue;
                }

                // 服务器错误
                if (strcmp(salt, "ERROR") == 0) {
                    printf("Server error, please try again\n");
                    continue;
                }

                // 输入密码
                printf("Password: ");
                fflush(stdout);

                if (fgets(passwd, sizeof(passwd), stdin) == NULL) {
                    return -1;
                }

                passwd[strcspn(passwd, "\n")] = '\0';

                // 使用服务器发送的 salt 对密码进行加密
                char *ciphertext = crypt(passwd, salt);

                if (ciphertext == NULL) {
                    perror("crypt");
                    return -1;
                }

                // 将密码密文发送给服务器
                if (sendResponse(sfd, ciphertext) == -1) {
                    perror("sendResponse ciphertext");
                    return -1;
                }

                // 接收服务器验证结果
                if (recvResponse(sfd,
                                 response,
                                 sizeof(response)) <= 0) {
                    perror("recvResponse");
                    return -1;
                }

                // 登录成功
                if (strcmp(response, "OK") == 0) {

                    printf("Login success\n");

                    return 0;
                }

                // 密码错误
                if (strcmp(response, "PASSWORD_ERROR") == 0) {

                    printf("Password error, please try again\n");

                    continue;
                }

                // 登录失败
                printf("Login failed, please try again\n");
            }
        }

        // 注册结束后重新显示菜单
    }
    return 0;
}
