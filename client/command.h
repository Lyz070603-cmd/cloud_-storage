#ifndef __COMMAND__
#define __COMMAND__

#include "protocol.h"
#include "function.h"
int cdFunction(int sfd,command_t *command,char * argv[],int argc);
#endif
