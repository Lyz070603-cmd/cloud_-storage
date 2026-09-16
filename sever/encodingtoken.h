#ifndef __ENCODINGTOKEN__
#define __ENCODINGTOKEN__

#include <l8w8jwt/encode.h>
#include <l8w8jwt/decode.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// JWT(HS512) 输出的最大缓冲，够放 header.payload.signature
#define TOKEN_BUF_SIZE 512

// 用 username 作为密钥生成 JWT，写入 token（tokenSize 字节），成功返回 0
int createToken(const char *username, char *token, size_t tokenSize);

// 用 username 作为密钥校验 token，有效返回 0，否则返回 -1
int verifyToken(const char *username, const char *token);

#endif
