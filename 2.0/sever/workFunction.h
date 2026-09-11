#include <function.h>
#include"clientInfo.h"
#include"protocol.h"
#include "command.h"
#include "logrecord.h"
int workFunc(clientInfo_t * clientInfo);
int handleCd(clientInfo_t *client,const char *path);
int handleLs(clientInfo_t *client,const char *path);
int handlePwd(clientInfo_t *client);
int handlePuts(clientInfo_t *client,const char *filename);
int handleGets(clientInfo_t *client,const char *filename);
int handleRemove(clientInfo_t *client,const char *filename);
