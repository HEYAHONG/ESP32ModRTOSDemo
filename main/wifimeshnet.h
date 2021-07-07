#ifndef WIFIMESHNET_H_INCLUDED
#define WIFIMESHNET_H_INCLUDED

#include "appconfig.h"
#include "stdint.h"
#include "stdbool.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"

#if CONFIG_WIFI_MESH_NETWORK == 1

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct
{
     //接收raw mesh数据,另一node采用esp_mesh_send发送时,发送数据的proto字段置为MESH_PROTO_BIN时由此函数接收
     void  (*receive_callback)(mesh_addr_t *from, mesh_data_t *data);

     //处理mesh事件
     void (*mesh_event_handler)(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);

     //处理IP事件,一般分配到ip地址后可视为已联网
     void (*ip_event_handler)(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);


} wifimeshnet_callback_t;


//初始化wifimeshnet
void wifimeshnet_init(wifimeshnet_callback_t callback);

typedef struct
{
    uint8_t mesh_id[6];
    uint8_t mesh_ap_password[64];
    size_t  mesh_ap_max_connections;
    int mesh_channel;
    uint8_t router_ssid[32];
    size_t  router_ssid_len;
    uint8_t router_password[64];
} wifimeshnet_config_t;
//设置配置参数
void wifimeshnet_set_config(wifimeshnet_config_t *cfg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CONFIG_WIFI_MESH_NETWORK

#endif // WIFIMESHNET_H_INCLUDED
