#ifndef __LOGRECORD__
#define __LOGRECORD__

#include <syslog.h>

// 初始化日志
#define LOG_INIT() \
    openlog("cloud_server", LOG_PID | LOG_CONS, LOG_USER)

// 关闭日志
#define LOG_CLOSE() \
    closelog()

// 记录用户操作日志
#define LOG(user, action, ret)                                      \
    do {                                                            \
        if ((ret) == 0) {                                           \
            syslog(LOG_INFO,                                        \
                   "user=%s action=%s ret=%d file=%s function=%s line=%d",\
                   (user),                                          \
                   (action),                                        \
                   (ret),                                           \
                   __FILE__,                                        \
                   __func__,                                    \
                   __LINE__);                                       \
        } else {                                                    \
            syslog(LOG_ERR,                                         \
                   "user=%s action=%s ret=%d file=%s function=%s line=%d",\
                   (user),                                          \
                   (action),                                        \
                   (ret),                                           \
                   __FILE__,                                        \
                   __func__,                                    \
                   __LINE__);                                       \
        }                                                           \
    } while (0)

#endif
