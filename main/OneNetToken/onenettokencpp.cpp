#include "onenettokencpp.h"

std::string OneNETTokenGetDeviceRes(std::string pruductid,std::string devicename)
{
    return std::string("products/")+pruductid+"/devices/"+devicename;
}

std::string OneNETTokenGetProductRes(std::string pruductid)
{
    return std::string("products/")+pruductid;
}

std::string OneNETTokenGetSign(time_t _et,std::string res,std::string key,std::string method,std::string version)
{
    std::string sign;
    if(res.empty() || method.empty() || version.empty())
    {
        return sign;
    }
    std::string et=std::to_string(_et);
    std::string to_sign=et+"\n"+method+"\n"+res+"\n"+version;
    std::string key_bin;
    {
        uint8_t key_blob[key.length()]= {0};
        size_t key_blob_length=OneNETBase64Decode(key_blob,sizeof(key_blob),(uint8_t *)key.c_str(),key.length());
        if(key_blob_length == 0)
        {
            return sign;
        }
        key_bin=std::string((char *)key_blob,key_blob_length);
    }
    {
        uint8_t sign_blob[ONENET_HMAC_OUT_MAX]= {0};
        size_t sign_blob_length=OneNETHmac(OneNETTokenCryptoMethodFromString(method.c_str()),(uint8_t *)sign_blob,sizeof(sign_blob),(uint8_t *) key_bin.c_str(),key_bin.length(),(uint8_t *)to_sign.c_str(),to_sign.length());
        if(sign_blob_length == 0)
        {
            return sign;
        }
        uint8_t sign_base64[ONENET_HMAC_OUT_MAX*4/3+4]= {0};
        OneNETBase64Encode(sign_base64,sizeof(sign_base64),sign_blob,sign_blob_length);
        sign=std::string((char *)sign_base64);
    }

    return sign;

}


static bool  StringReplace(std::string &val,std::string old_str,std::string new_str)
{
    std::string::size_type pos=val.find(old_str);
    if(pos==std::string::npos)
    {
        return false;
    }

    val.replace(pos,old_str.length(),new_str);

    return true;
}
static void OneNETTokenUrlConvert(std::string &val)
{
    while(StringReplace(val,"%","%25"));
    while(StringReplace(val,"+","%2B"));
    while(StringReplace(val," ","%20"));
    while(StringReplace(val,"/","%2F"));
    while(StringReplace(val,"?","%3F"));
    while(StringReplace(val,"#","%23"));
    while(StringReplace(val,"&","%26"));
    while(StringReplace(val,"=","%3D"));
}

std::string OneNETTokenGenerateURLToken(time_t _et,std::string res,std::string sign,std::string method,std::string version)
{
    std::string et=std::to_string(_et);
    OneNETTokenUrlConvert(res);
    if(sign.empty())
    {
        return std::string("");
    }
    OneNETTokenUrlConvert(sign);
    OneNETTokenUrlConvert(version);
    OneNETTokenUrlConvert(method);

    return std::string("version=")+version+"&res="+res+"&et="+et+"&method="+method+"&sign="+sign;
}

