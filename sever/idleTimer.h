
// 30 秒环形队列：定时踢出不活跃用户。
//
// 结构：30 个格子（每个格子 1 秒），游标每秒前进一格。
// 用户活跃时（收到消息）：从旧格子删除，插入游标当前格子的尾部（重置计时）。
// 游标前进到某格时：该格子里若还有用户，说明该用户已整整 30 秒不活跃，予以踢出。

#ifndef IDLE_TIMER_H
#define IDLE_TIMER_H

#include "clientInfo.h"

#define IDLE_SLOTS 30        // 环形队列格子数 = 30 秒
#define IDLE_TIMEOUT_SEC 30  // 30 秒不活跃即踢出

typedef struct {
    idleNode_t slots[IDLE_SLOTS]; // 每个格子是一个哨兵头节点（环形双向链表）
    int cursor;                    // 当前游标指向的格子下标
} idleTimer_t;

// 初始化：所有格子自环，游标指向 0
void idleTimerInit(idleTimer_t *t);

// 用户活跃时调用：删除旧记录（若在队列中）+ 插入当前格子（增/删）
void idleTimerTouch(idleTimer_t *t, clientInfo_t *ci);

// 将用户从队列中彻底移除（连接关闭/断连时调用），幂等
void idleTimerRemove(idleTimer_t *t, clientInfo_t *ci);

// 游标前进一格，并清理新指向的格子；把其中已 30 秒不活跃的用户
// 逐个写入 expired（最多 max 个），返回取出的个数
int idleTimerAdvance(idleTimer_t *t, clientInfo_t **expired, int max);

// 查询：用户当前所在格子下标，-1 表示不在队列中
int idleTimerSlot(const clientInfo_t *ci);

#endif
