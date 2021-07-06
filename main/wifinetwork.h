#ifndef WIFINETWORK_H_INCLUDED
#define WIFINETWORK_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "appconfig.h"
#include "esp_smartconfig.h"
#include "stdint.h"


typedef struct
{
#ifndef CONFIG_WIFI_NETWORK_SOFTAP
//STA ���״̬
    bool station_is_enable;
    int station_retry_num;
    bool station_is_connect_ap;
    bool station_is_connect_ap_ever;//�����ӹ�
    bool station_is_in_smartconfig;
#endif // CONFIG_WIFI_NETWORK_SOFTAP

#ifndef CONFIG_WIFI_NETWORK_STA
//AP ���״̬
    bool ap_is_enable;
    int ap_station_count;
#endif // CONFIG_WIFI_NETWORK_STA
    bool wifinetwork_running;
} wifinetwork_state_t;

//��ʼ��wifinetwork
void wifinetwork_init();


//ֹͣwifinetowrk
void wifinetwork_stop();

//����wifinetowrk
void wifinetwork_start();

wifinetwork_state_t wifinetwork_getstate();

#ifndef CONFIG_WIFI_NETWORK_SOFTAP
//����STA��ز���
void wifinetwork_station_set_config(const char * ssid,const char * password);

//����smartconfig
void wifinetwork_sta_smartconfg_start(smartconfig_type_t sctype,size_t timeout_s);

//ֹͣsmartconfig
void wifinetwork_sta_smartconfg_stop();

#endif // CONFIG_WIFI_NETWORK_SOFTAP

#ifndef CONFIG_WIFI_NETWORK_STA
//����AP��ز���
void wifinetwork_ap_set_config(const char * ssid,const char * password);

#endif // CONFIG_WIFI_NETWORK_STA




#ifdef __cplusplus
};
#endif // __cplusplus

#endif // WIFINETWORK_H_INCLUDED
