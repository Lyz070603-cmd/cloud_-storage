#include "encodingtoken.h"

int createToken(const char *username, char *token, size_t tokenSize) {
    if (username == NULL || token == NULL || tokenSize == 0) {
        return -1;
    }

    char *jwt = NULL;
    size_t jwt_length = 0;

    struct l8w8jwt_encoding_params params;
    l8w8jwt_encoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;
    params.sub = "Netdisk";
    params.iss = "Lyz";
    params.aud = "client";
    params.iat = 0;
    params.exp = 0x7fffffff;

    params.secret_key = (unsigned char*)username;
    params.secret_key_length = strlen(username);

    params.out = &jwt;
    params.out_length = &jwt_length;

    int r = l8w8jwt_encode(&params);
    if (r != L8W8JWT_SUCCESS) {
        if (jwt != NULL) {
            l8w8jwt_free(jwt);
        }
        return -1;
    }

    if (jwt_length + 1 > tokenSize) {
        l8w8jwt_free(jwt);
        return -1;
    }

    memcpy(token, jwt, jwt_length);
    token[jwt_length] = '\0';
    l8w8jwt_free(jwt);
    return 0;
}

int verifyToken(const char *username, const char *token) {
    if (username == NULL || token == NULL) {
        return -1;
    }

    struct l8w8jwt_decoding_params params;
    l8w8jwt_decoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;

    params.jwt = (char*)token;
    params.jwt_length = strlen(token);

    params.verification_key = (unsigned char*)username;
    params.verification_key_length = strlen(username);

    params.validate_iss = "Lyz";
    params.validate_sub = "Netdisk";

    params.validate_exp = 0;
    params.exp_tolerance_seconds = 60;

    params.validate_iat = 0;
    params.iat_tolerance_seconds = 60;

    enum l8w8jwt_validation_result validation_result;
    int decode_result = l8w8jwt_decode(&params, &validation_result, NULL, NULL);

    if (decode_result == L8W8JWT_SUCCESS && validation_result == L8W8JWT_VALID) {
        return 0;
    }
    return -1;
}
