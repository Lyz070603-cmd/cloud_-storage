#ifndef __COMMAND__
#define __COMMAND__

#define MMAP_SIZE (100 * 1024 * 1024)

#include "protocol.h"
#include <errno.h>
#include "function.h"

int cdFunction(int sfd,command_t *command,char * argv[],int argc);
int pwdFunction(int sfd,command_t *command,char * argv[],int argc);
int lsFunction(int sfd,command_t *command,char * argv[],int argc);
int getsFunction(int sfd,command_t *command,char * argv[],int argc);
int putsFunction(int sfd, command_t *command, char *argv[], int argc);
int removeFunction(int sfd, command_t *command, char *argv[], int argc);
#endif
