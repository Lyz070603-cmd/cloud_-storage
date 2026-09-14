#include "command.h"
#include "protocol.h"

// 根据路径栈构造完整虚拟路径（例如 "/a/b"）
static void buildPath(clientInfo_t *client, char *out, size_t outSize) {
    if (client->cwdTop == 0) {
        snprintf(out, outSize, "/");
        return;
    }
    size_t pos = 0;
    out[pos++] = '/';
    for (int i = 0; i < client->cwdTop; ++i) {
        int n = snprintf(out + pos, outSize - pos, "%s", client->cwdStack[i]);
        if (n < 0 || (size_t)n >= outSize - pos) break;
        pos += n;
        if (i < client->cwdTop - 1 && pos < outSize - 1) {
            out[pos++] = '/';
        }
    }
    out[pos] = '\0';
}

// 在 parentId 目录下查找名字为 name 的未删除项
// 返回：1 找到（填 outId/outType/outSha1），0 未找到，-1 出错
static int findChildIn(clientInfo_t *client, int parentId, const char *name,
                       int *outId, char *outType, char *outSha1) {
    char esc[2 * MAX_NAME_LEN + 1];
    mysql_real_escape_string(client->mysql, esc, name, (unsigned long)strlen(name));

    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id,type,sha1 FROM User WHERE pre_id=%d AND filename='%s' AND tomb=0",
             parentId, esc);

    if (mysql_query(client->mysql, sql) != 0) return -1;
    MYSQL_RES *res = mysql_store_result(client->mysql);
    if (res == NULL) return -1;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == NULL) {
        mysql_free_result(res);
        return 0;
    }
    if (outId) *outId = atoi(row[0]);
    if (outType) {
        strncpy(outType, row[1] ? row[1] : "", 15);
        outType[15] = '\0';
    }
    if (outSha1) {
        strncpy(outSha1, row[2] ? row[2] : "", 40);
        outSha1[40] = '\0';
    }
    mysql_free_result(res);
    return 1;
}

// 解析路径表达式（支持 /a/b、a/b、.、..、a/../b、a//b 等）
static int resolveEntry(clientInfo_t *client, const char *path,
                        char outStack[MAX_DEPTH][MAX_NAME_LEN],
                        int outIdStack[MAX_DEPTH],
                        int *outTop,
                        int *outId, char *outType, char *outSha1) {
    // 初始状态：绝对路径从根开始，相对路径复制当前栈
    int top = 0;
    int isAbs = (path != NULL && path[0] == '/');
    if (isAbs) {
        top = 0;
    } else {
        top = client->cwdTop;
        for (int i = 0; i < top; ++i) {
            strncpy(outStack[i], client->cwdStack[i], MAX_NAME_LEN - 1);
            outStack[i][MAX_NAME_LEN - 1] = '\0';
            outIdStack[i] = client->cwdIdStack[i];
        }
    }
    int curId = (top > 0) ? outIdStack[top - 1] : client->rootId;

    // 空、"."、"/" 特判
    if (path == NULL || strlen(path) == 0 ||
        strcmp(path, ".") == 0 || strcmp(path, "/") == 0) {
        *outTop = top;
        *outId = curId;
        strcpy(outType, "dir");
        outSha1[0] = '\0';
        return 1;
    }

    char buf[1024];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *segs[MAX_DEPTH + 2];
    int nseg = 0;
    char *saveptr = NULL;
    char *tok = strtok_r(buf, "/", &saveptr);
    while (tok != NULL && nseg < MAX_DEPTH + 2) {
        segs[nseg++] = tok;
        tok = strtok_r(NULL, "/", &saveptr);
    }

    if (nseg == 0) {
        *outTop = top;
        *outId = curId;
        strcpy(outType, "dir");
        outSha1[0] = '\0';
        return 1;
    }

    // 前 nseg-1 段都必须是目录
    for (int i = 0; i < nseg - 1; ++i) {
        const char *seg = segs[i];
        if (strcmp(seg, ".") == 0) continue;
        if (strcmp(seg, "..") == 0) {
            if (top > 0) { top--; curId = (top > 0) ? outIdStack[top - 1] : client->rootId; }
            continue;
        }
        int id = 0; char type[16] = {0};
        int r = findChildIn(client, curId, seg, &id, type, NULL);
        if (r != 1 || strcmp(type, "dir") != 0) return 0;
        if (top >= MAX_DEPTH) return -1;
        strncpy(outStack[top], seg, MAX_NAME_LEN - 1);
        outStack[top][MAX_NAME_LEN - 1] = '\0';
        outIdStack[top] = id;
        top++;
        curId = id;
    }

    // 最后一段
    const char *last = segs[nseg - 1];
    if (strcmp(last, ".") == 0) {
        *outTop = top;
        *outId = curId;
        strcpy(outType, "dir");
        outSha1[0] = '\0';
        return 1;
    }
    if (strcmp(last, "..") == 0) {
        if (top > 0) { top--; curId = (top > 0) ? outIdStack[top - 1] : client->rootId; }
        *outTop = top;
        *outId = curId;
        strcpy(outType, "dir");
        outSha1[0] = '\0';
        return 1;
    }

    int id = 0; char type[16] = {0}; char sha1Hex[41] = {0};
    int r = findChildIn(client, curId, last, &id, type, sha1Hex);
    if (r != 1) return 0;

    if (strcmp(type, "dir") == 0) {
        if (top >= MAX_DEPTH) return -1;
        strncpy(outStack[top], last, MAX_NAME_LEN - 1);
        outStack[top][MAX_NAME_LEN - 1] = '\0';
        outIdStack[top] = id;
        top++;
        *outTop = top;
        *outId = id;
        strcpy(outType, "dir");
        outSha1[0] = '\0';
    } else {
        *outTop = top;
        *outId = id;
        strcpy(outType, "file");
        strncpy(outSha1, sha1Hex, 40);
        outSha1[40] = '\0';
    }
    return 1;
}

// 在当前目录插入或更新一个文件项（秒传与真实上传共用）
static int upsertFile(clientInfo_t *client, const char *filename, const char *sha1Hex) {
    char fnEsc[2 * MAX_NAME_LEN + 1];
    mysql_real_escape_string(client->mysql, fnEsc, filename, (unsigned long)strlen(filename));

    char dirPath[1024];
    buildPath(client, dirPath, sizeof(dirPath));

    char fullPath[1200];
    if (strcmp(dirPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "/%s", filename);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, filename);
    }
    char pathEsc[2 * sizeof(fullPath) + 1];
    mysql_real_escape_string(client->mysql, pathEsc, fullPath, (unsigned long)strlen(fullPath));

    char shaEsc[2 * 40 + 1];
    mysql_real_escape_string(client->mysql, shaEsc, sha1Hex, (unsigned long)strlen(sha1Hex));

    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id FROM User WHERE pre_id=%d AND filename='%s' AND type='file' AND tomb=0",
             client->curDirId, fnEsc);

    if (mysql_query(client->mysql, sql) != 0) return -1;
    MYSQL_RES *res = mysql_store_result(client->mysql);
    if (res == NULL) return -1;
    MYSQL_ROW row = mysql_fetch_row(res);
    int exists = (row != NULL);
    int fid = exists ? atoi(row[0]) : 0;
    mysql_free_result(res);

    if (exists) {
        snprintf(sql, sizeof(sql),
                 "UPDATE User SET sha1='%s', tomb=0 WHERE id=%d", shaEsc, fid);
    } else {
        snprintf(sql, sizeof(sql),
                 "INSERT INTO User(name,filename,pre_id,path,type,sha1,tomb,password,salt) "
                 "VALUES(NULL,'%s',%d,'%s','file','%s',0,'',NULL)",
                 fnEsc, client->curDirId, pathEsc, shaEsc);
    }
    return mysql_query(client->mysql, sql);
}

// 递归软删除（把 id 及其所有子孙 tomb 置 1）
static void tombRecursive(clientInfo_t *client, int id) {
    int stack[4096];
    int top = 0;
    stack[top++] = id;
    while (top > 0) {
        int cur = stack[--top];
        char sql[256];
        snprintf(sql, sizeof(sql), "UPDATE User SET tomb=1 WHERE id=%d", cur);
        mysql_query(client->mysql, sql);

        snprintf(sql, sizeof(sql), "SELECT id FROM User WHERE pre_id=%d AND tomb=0", cur);
        if (mysql_query(client->mysql, sql) != 0) continue;
        MYSQL_RES *res = mysql_store_result(client->mysql);
        if (res == NULL) continue;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != NULL) {
            if (top < (int)(sizeof(stack) / sizeof(stack[0]))) {
                stack[top++] = atoi(row[0]);
            }
        }
        mysql_free_result(res);
    }
}

// 列出 dirId 的子项
static int listDirContents(clientInfo_t *client, int dirId) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT filename,type FROM User WHERE pre_id=%d AND tomb=0 ORDER BY type,filename",
             dirId);
    if (mysql_query(client->mysql, sql) != 0) {
        sendResponse(client->netfd, "ERROR\n");
        return -1;
    }
    MYSQL_RES *res = mysql_store_result(client->mysql);
    if (res == NULL) {
        sendResponse(client->netfd, "ERROR\n");
        return -1;
    }

    char out[4096] = {0};
    size_t used = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        if (used >= sizeof(out) - 1) break;
        const char *name = row[0] ? row[0] : "";
        const char *type = row[1] ? row[1] : "";
        used += (size_t)snprintf(out + used, sizeof(out) - used, "%s%s ",
                                 name, strcmp(type, "dir") == 0 ? "/" : "");
    }
    if (used < sizeof(out) - 1) {
        out[used++] = '\n';
        out[used] = '\0';
    }
    mysql_free_result(res);

    sendResponse(client->netfd, out);
    return 0;
}

int handleCd(clientInfo_t *client, const char *path) {
    char tmpStack[MAX_DEPTH][MAX_NAME_LEN];
    int tmpIds[MAX_DEPTH];
    int tmpTop = 0;
    int id = 0; char type[16] = {0}; char sha1Hex[41] = {0};

    int r = resolveEntry(client, path, tmpStack, tmpIds, &tmpTop, &id, type, sha1Hex);
    if (r != 1 || strcmp(type, "dir") != 0) {
        sendResponse(client->netfd, "Directory does not exist\n");
        return -1;
    }

    // 提交：把解析结果复制回 client
    client->cwdTop = tmpTop;
    for (int i = 0; i < tmpTop; ++i) {
        strncpy(client->cwdStack[i], tmpStack[i], MAX_NAME_LEN - 1);
        client->cwdStack[i][MAX_NAME_LEN - 1] = '\0';
        client->cwdIdStack[i] = tmpIds[i];
    }
    client->curDirId = id;
    sendResponse(client->netfd, "OK\n");
    return 0;
}

int handleLs(clientInfo_t *client, const char *path) {
    char tmpStack[MAX_DEPTH][MAX_NAME_LEN];
    int tmpIds[MAX_DEPTH];
    int tmpTop = 0;
    int id = 0; char type[16] = {0}; char sha1Hex[41] = {0};

    int r = resolveEntry(client, path, tmpStack, tmpIds, &tmpTop, &id, type, sha1Hex);
    if (r != 1) {
        sendResponse(client->netfd, "Directory does not exist\n");
        return 0;
    }

    if (strcmp(type, "file") == 0) {
        const char *base = path;
        const char *slash = strrchr(path, '/');
        if (slash != NULL) base = slash + 1;
        char res[MAX_NAME_LEN + 2];
        snprintf(res, sizeof(res), "%s\n", base);
        sendResponse(client->netfd, res);
        return 0;
    }

    return listDirContents(client, id);
}

int handlePwd(clientInfo_t *client) {
    char path[1024];
    buildPath(client, path, sizeof(path));
    char res[1100];
    snprintf(res, sizeof(res), "%s\n", path);
    sendResponse(client->netfd, res);
    return 0;
}

int handlePuts(clientInfo_t *client, const char *filename) {
    // 1. 接收客户端计算好的 SHA1（40 位十六进制）
    char sha1Hex[41] = {0};
    int ret = recvResponse(client->netfd, sha1Hex, sizeof(sha1Hex));
    if (ret <= 0) {
        return -1;
    }

    char hashPath[512];
    snprintf(hashPath, sizeof(hashPath), "test/%s", sha1Hex);

    // 2. 秒传：test/ 下已存在相同哈希的文件，则不再接收内容
    if (access(hashPath, F_OK) == 0) {
        printf("秒传: %s (sha1=%s)\n", filename, sha1Hex);
        sendResponse(client->netfd, "SKIP");
        upsertFile(client, filename, sha1Hex);
        return 0;
    }

    // 3. 需要真实上传
    sendResponse(client->netfd, "READY");

    off_t filesize = 0;
    if (recvAll(client->netfd, &filesize, sizeof(filesize)) <= 0) {
        return -1;
    }

    int fd = open(hashPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        sendResponse(client->netfd, "ERROR");
        return -1;
    }

    char buf[8192];
    off_t received = 0;
    while (received < filesize) {
        size_t n = sizeof(buf);
        if (filesize - received < (off_t)n) {
            n = (size_t)(filesize - received);
        }
        if (recvAll(client->netfd, buf, n) <= 0) {
            close(fd);
            return -1;
        }
        if (write(fd, buf, n) != (ssize_t)n) {
            perror("write");
            close(fd);
            sendResponse(client->netfd, "ERROR");
            return -1;
        }
        received += (off_t)n;
    }
    close(fd);

    if (upsertFile(client, filename, sha1Hex) != 0) {
        sendResponse(client->netfd, "ERROR");
        return -1;
    }
    sendResponse(client->netfd, "OK");
    return 0;
}

int handleGets(clientInfo_t *client, const char *filename) {
    int id = 0; char type[16] = {0}; char sha1Hex[41] = {0};
    int r = findChildIn(client, client->curDirId, filename, &id, type, sha1Hex);
    if (r == -1) {
        sendResponse(client->netfd, "ERROR");
        return -1;
    }
    if (r == 0 || strcmp(type, "file") != 0) {
        sendResponse(client->netfd, "NOT_FOUND");
        return -1;
    }

    char hashPath[512];
    snprintf(hashPath, sizeof(hashPath), "test/%s", sha1Hex);

    int fd = open(hashPath, O_RDONLY);
    if (fd == -1) {
        perror("open");
        sendResponse(client->netfd, "ERROR");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1 || !S_ISREG(st.st_mode)) {
        perror("fstat");
        close(fd);
        sendResponse(client->netfd, "ERROR");
        return -1;
    }
    off_t filesize = st.st_size;

    sendResponse(client->netfd, "READY");
    if (sendAll(client->netfd, &filesize, sizeof(filesize)) == -1) {
        close(fd);
        return -1;
    }

    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (sendAll(client->netfd, buf, n) == -1) {
            close(fd);
            return -1;
        }
    }
    close(fd);

    sendResponse(client->netfd, "OK");
    return 0;
}

int handleRemove(clientInfo_t *client, const char *filename) {
    int id = 0; char type[16] = {0};
    int r = findChildIn(client, client->curDirId, filename, &id, type, NULL);
    if (r == -1) {
        sendResponse(client->netfd, "ERROR");
        return -1;
    }
    if (r == 0) {
        sendResponse(client->netfd, "ERROR");
        return -1;
    }

    tombRecursive(client, id);
    sendResponse(client->netfd, "OK");
    return 0;
}
