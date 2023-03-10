#ifndef ONENETTOKEN_H
#define ONENETTOKEN_H
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"


size_t OneNETBase64Decode(uint8_t *out,size_t outlen,const uint8_t *in,size_t inlen);
size_t OneNETBase64Encode(uint8_t *out,size_t outlen,const uint8_t *in,size_t inlen);

typedef enum
{
    ONENET_VERSION_2018_10_31=1,
    ONENET_VERSION_DEFAULT= ONENET_VERSION_2018_10_31
} OneNETTokenVersion;

OneNETTokenVersion OneNETTokenVersionFromString(const char * version);

typedef enum
{
    ONENET_CRYPTO_MD5=1,
    ONENET_CRYPTO_SHA1,
    ONENET_CRYPTO_SHA256,
    ONENET_CRYPTO_DEFAULT=ONENET_CRYPTO_SHA256
} OneNETTokenCryptoMethod;

OneNETTokenCryptoMethod OneNETTokenCryptoMethodFromString(const char * method);

#define ONENET_HMAC_OUT_MAX 64

size_t OneNETHmac(OneNETTokenCryptoMethod method,uint8_t *out,size_t outlen,const uint8_t *key,size_t keylen,const uint8_t *data,size_t datalen);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // ONENETTOKEN_H
