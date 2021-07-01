#ifndef WIFINETWORK_H_INCLUDED
#define WIFINETWORK_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "appconfig.h"

typedef struct
{
#ifndef CONFIG_WIFI_NETWORK_SOFTAP
//STA 相关状态
    bool station_is_enable;
    int station_retry_num;
    bool station_is_connect_ap;
#endif // CONFIG_WIFI_NETWORK_SOFTAP

#ifndef CONFIG_WIFI_NETWORK_STA
//AP 相关状态
    bool ap_is_enable;
    int ap_station_count;
#endif // CONFIG_WIFI_NETWORK_STA

} wifinetwork_state_t;

//初始化wifinetwork
void wifinetwork_init();


wifinetwork_state_t wifinetwork_getstate();

#ifndef CONFIG_WIFI_NETWORK_SOFTAP
//设置STA相关参数
void wifinetwork_station_set_config(const char * ssid,const char * password);
#endif // CONFIG_WIFI_NETWORK_SOFTAP

#ifndef CONFIG_WIFI_NETWORK_STA
//设置AP相关参数
void wifinetwork_ap_set_config(const char * ssid,const char * password);
#endif // CONFIG_WIFI_NETWORK_STA


#ifdef __cplusplus
};
#endif // __cplusplus

#endif // WIFINETWORK_H_INCLUDED
