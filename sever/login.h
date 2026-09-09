#ifndef __LOGIN__
#define __LOGIN__

#include "clientInfo.h"
#include "protocol.h"
#include <function.h>
#include <shadow.h>
#include "workFunction.h"
int loginhandle(clientInfo_t *clientInfo);
void get_salt(char *salt,char *passwd);
#endif
