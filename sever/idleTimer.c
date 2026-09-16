// idleTimer.c
//
// 30 秒环形队列实现。

#include "idleTimer.h"

#include <stddef.h>  // offsetof

// 由内嵌节点反推所属 clientInfo_t
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

// 哨兵头节点自环
static void nodeInit(idleNode_t *node){
    node->prev = node;
    node->next = node;
}

// 将 node 插到 sentinel 尾部（因是环形，即 sentinel->prev 之后）
static void nodePushBack(idleNode_t *sentinel, idleNode_t *node){
    node->next = sentinel;
    node->prev = sentinel->prev;
    sentinel->prev->next = node;
    sentinel->prev = node;
}

// 从链表中摘除 node
static void nodeUnlink(idleNode_t *node){
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
}

// 是否已挂在某个链表上
static int nodeIsLinked(const idleNode_t *node){
    return node->next != NULL;
}

void idleTimerInit(idleTimer_t *t){
    for (int i = 0; i < IDLE_SLOTS; ++i){
        nodeInit(&t->slots[i]);
    }
    t->cursor = 0;
}

void idleTimerTouch(idleTimer_t *t, clientInfo_t *ci){
    if (t == NULL || ci == NULL) return;

    // 删：若已在队列中，先从旧格子摘除
    idleTimerRemove(t, ci);

    // 增：插入当前格子尾部
    idleNode_t *slot = &t->slots[t->cursor];
    nodePushBack(slot, &ci->idleNode);
    ci->idleSlot = t->cursor;
}

void idleTimerRemove(idleTimer_t *t, clientInfo_t *ci){
    (void)t;
    if (ci == NULL) return;

    if (nodeIsLinked(&ci->idleNode)){
        nodeUnlink(&ci->idleNode);
    }
    ci->idleSlot = -1;
}

int idleTimerAdvance(idleTimer_t *t, clientInfo_t **expired, int max){
    // 游标前进一格
    t->cursor = (t->cursor + 1) % IDLE_SLOTS;
    idleNode_t *slot = &t->slots[t->cursor];

    int count = 0;
    idleNode_t *node = slot->next;
    while (node != slot){
        idleNode_t *next = node->next; // 先保存后继，摘除后 node 会被置空
        clientInfo_t *ci = container_of(node, clientInfo_t, idleNode);
        nodeUnlink(node);
        ci->idleSlot = -1;

        if (expired != NULL && count < max){
            expired[count] = ci;
        }
        ++count;

        node = next;
    }
    return count;
}

int idleTimerSlot(const clientInfo_t *ci){
    return ci == NULL ? -1 : ci->idleSlot;
}
