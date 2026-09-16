#ifndef __IOGIN__
#define __IOGIN__

#include "function.h"
#include "protocol.h"
#define SIGNUP 1
#define LOGIN 2

int loginFunction(int sfd);

// 登录成功后记录用户名（main.c 里定义）
extern char g_username[64];
#endif
