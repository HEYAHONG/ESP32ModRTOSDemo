#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include "stdint.h"
#include "stdlib.h"
#include "string.h"

extern char macstr[20];//mac地址字符串

void app_init();

void app_loop();

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // APP_H_INCLUDED
