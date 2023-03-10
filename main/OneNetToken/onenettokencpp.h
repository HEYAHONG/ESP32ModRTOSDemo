#ifndef ONENETTOKENCPP_H
#define ONENETTOKENCPP_H
#include "onenettoken.h"
#ifdef __cplusplus
#include <string>
#include <time.h>
#include <string.h>

#ifndef ONENETCPP_DEFAULT_VERSION
#define ONENETCPP_DEFAULT_VERSION "2018-10-31"
#endif // ONENETCPP_DEFAULT_VERSION

#ifndef ONENETCPP_DEFAULT_METHOD
#define ONENETCPP_DEFAULT_METHOD "sha256"
#endif // ONENETCPP_DEFAULT_METHOD

std::string OneNETTokenGetDeviceRes(std::string pruductid,std::string devicename);

std::string OneNETTokenGetProductRes(std::string pruductid);

std::string OneNETTokenGetSign(time_t _et,std::string res,std::string key,std::string method=ONENETCPP_DEFAULT_METHOD,std::string version=ONENETCPP_DEFAULT_VERSION);


std::string OneNETTokenGenerateURLToken(time_t _et,std::string res,std::string sign,std::string method=ONENETCPP_DEFAULT_METHOD,std::string version=ONENETCPP_DEFAULT_VERSION);

#endif // __cplusplus

#endif // ONENETTOKENCPP_H
