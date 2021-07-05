#ifndef WIFIMESHNET_H_INCLUDED
#define WIFIMESHNET_H_INCLUDED

#include "appconfig.h"
#include "stdint.h"
#include "stdbool.h"

#if CONFIG_WIFI_MESH_NETWORK == 1

#ifdef __cplusplus
extern "C"
{
#endif

//初始化wifimeshnet
void wifimeshnet_init();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CONFIG_WIFI_MESH_NETWORK

#endif // WIFIMESHNET_H_INCLUDED
