// clientInfo.h

#ifndef CLIENT_INFO_H
#define CLIENT_INFO_H

#include <mysql/mysql.h>

// 虚拟路径最大深度 / 名字最大长度
#define MAX_DEPTH 64
#define MAX_NAME_LEN 64

// 不活跃计时环形队列使用的双向链表节点（内嵌在 clientInfo_t 中）
typedef struct idleNode {
    struct idleNode *prev;
    struct idleNode *next;
} idleNode_t;

typedef struct {
    int netfd;
    struct MYSQL *mysql;

    int rootId;                             // 用户根目录行的 id（即该用户）
    int curDirId;                           // 当前目录行的 id

    char cwdStack[MAX_DEPTH][MAX_NAME_LEN]; // 路径栈：从根开始的目录名序列
    int  cwdIdStack[MAX_DEPTH];             // 与上面平行的目录行 id 栈
    int  cwdTop;                            // 栈中目录个数

    char name[64];                          // 用户名

    // ---- 不活跃计时（环形队列）----
    idleNode_t idleNode;                    // 所在格子的链表节点
    int idleSlot;                           // 当前所在格子下标，-1 表示不在队列中
} clientInfo_t;

#endif
