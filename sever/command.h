#ifndef COMMAND_H
#define COMMAND_H

#define MMAP_SIZE (100 * 1024 * 1024)

#include "function.h"
#include "clientInfo.h"

int handleCd(clientInfo_t *client, const char *path);

// 解析路径并提交到 client（不发送响应）；成功返回 0，失败返回 -1
int resolveAndCommit(clientInfo_t *client, const char *path);

// 把当前虚拟路径写入 out（例如 "/a/b"）
void getCwdPath(clientInfo_t *client, char *out, size_t outSize);

int handleLs(clientInfo_t *client,const char *path);

int handlePwd(clientInfo_t *client);

int handlePuts(clientInfo_t *client, const char *filename);

int handleGets(clientInfo_t *client, const char *filename);

int handleRemove(clientInfo_t *client, const char *filename);

#endif
