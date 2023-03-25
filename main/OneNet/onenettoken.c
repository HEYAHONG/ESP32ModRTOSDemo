#include "onenettoken.h"
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

size_t OneNETBase64Decode(uint8_t *out,size_t outlen,const uint8_t *in,size_t inlen)
{
    if(out==NULL || outlen==0 || in==NULL || inlen==0)
    {
        return 0;
    }
    size_t retlen=0;
    if(0==mbedtls_base64_decode(out,outlen,&retlen,in,inlen))
    {
        return retlen;
    }
    else
    {
        return 0;
    }
}

size_t OneNETBase64Encode(uint8_t *out,size_t outlen,const uint8_t *in,size_t inlen)
{
    if(out==NULL || outlen==0 || in==NULL || inlen==0)
    {
        return 0;
    }
    size_t retlen=0;
    if(0==mbedtls_base64_encode(out,outlen,&retlen,in,inlen))
    {
        return retlen;
    }
    else
    {
        return 0;
    }
}

OneNETTokenVersion OneNETTokenVersionFromString(const char * version)
{
    OneNETTokenVersion ret=ONENET_VERSION_DEFAULT;
    if(version!=NULL)
    {
        if(0==strcmp("2018-10-31",version))
        {
            ret=ONENET_VERSION_2018_10_31;
        }
    }
    return ret;
}

OneNETTokenCryptoMethod OneNETTokenCryptoMethodFromString(const char * method)
{
    OneNETTokenCryptoMethod ret=ONENET_CRYPTO_DEFAULT;
    if(method!=NULL)
    {
        if(0==strcmp("md5",method))
        {
            ret=ONENET_CRYPTO_MD5;
        }
        if(0==strcmp("sha1",method))
        {
            ret=ONENET_CRYPTO_SHA1;
        }
        if(0==strcmp("sha256",method))
        {
            ret=ONENET_CRYPTO_SHA256;
        }
    }
    return ret;
}

size_t OneNETHmac(OneNETTokenCryptoMethod method,uint8_t *out,size_t outlen,const uint8_t *key,size_t keylen,const uint8_t *data,size_t datalen)
{
    if(out==NULL || outlen==0 ||key==NULL ||keylen==0||data==NULL||datalen==0)
    {
        return 0;
    }
    if(outlen<MBEDTLS_MD_MAX_SIZE)
    {
        return 0;
    }
    mbedtls_md_type_t alg = MBEDTLS_MD_SHA256;
    switch(method)
    {
    case ONENET_CRYPTO_MD5:
    {
        alg=MBEDTLS_MD_MD5;
    }
    break;
    case ONENET_CRYPTO_SHA1:
    {
        alg=MBEDTLS_MD_SHA1;
    }
    break;
    case ONENET_CRYPTO_SHA256:
    {
        alg=MBEDTLS_MD_SHA256;
    }
    break;
    default:
    {
        alg=MBEDTLS_MD_SHA256;
    }
    break;
    }
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(alg);
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    if(0!=mbedtls_md_setup(&ctx, info,1))
    {
        mbedtls_md_free(&ctx);
        return 0;
    }
    if(0!=mbedtls_md_hmac_starts(&ctx,key,keylen))
    {
        mbedtls_md_free(&ctx);
        return 0;
    }
    if(0!=mbedtls_md_hmac_update(&ctx,data,datalen))
    {
        mbedtls_md_free(&ctx);
        return 0;
    }
    if(0!=mbedtls_md_hmac_finish(&ctx,out))
    {
        mbedtls_md_free(&ctx);
        return 0;
    }

    mbedtls_md_free(&ctx);
    return mbedtls_md_get_size(info);

}


