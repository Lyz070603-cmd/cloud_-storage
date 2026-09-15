#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

/*
 * 参数个数检查：argc 必须等于 num，否则打印错误并返回 -1
 */
#define ARGS_CHECK(argc, num)                             \
    do {                                                  \
        if ((argc) != (num)) {                            \
            fprintf(stderr, "args error!\n");             \
            return -1;                                    \
        }                                                 \
    } while (0)

/*
 * 系统调用返回值检查：ret == retval 表示出错，打印错误并返回 -1
 */
#define ERROR_CHECK(ret, retval, msg)                     \
    do {                                                  \
        if ((ret) == (retval)) {                          \
            perror(msg);                                  \
            return -1;                                    \
        }                                                 \
    } while (0)

#endif
