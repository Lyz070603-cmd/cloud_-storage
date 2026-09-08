#ifndef COMMAND_H
#define COMMAND_H

#include<function.h>
#include "clientInfo.h"

int handleCd(clientInfo_t *client, const char *path);

int handleLs(clientInfo_t *client,const char *path);

int handlePwd(clientInfo_t *client);

int handlePuts(clientInfo_t *client, const char *filename);

int handleGets(clientInfo_t *client, const char *filename);

int handleRemove(clientInfo_t *client, const char *filename);

#endif
