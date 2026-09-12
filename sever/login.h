#ifndef __LOGIN__
#define __LOGIN__

#include "logrecord.h"
#include "clientInfo.h"
#include "protocol.h"
#include <function.h>
#include <shadow.h>
#include "workFunction.h"
#include <mysql/mysql.h>

#define SIGNUP 1
#define LOGIN 2
#define SALT_LEN 8
int loginhandle(clientInfo_t *clientInfo);
int generateSalt(char *salt);
#endif
