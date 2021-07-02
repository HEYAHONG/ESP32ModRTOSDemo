#include "wifinetwork.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"
#include "stdbool.h"
#include "init.h"

static const char *TAG = "wifi network";

static volatile wifinetwork_state_t wifinetworkstate={0};

#ifndef CONFIG_WIFI_NETWORK_SOFTAP


static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        wifinetworkstate.station_is_connect_ap=false;
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifinetworkstate.station_is_connect_ap=false;
        if (wifinetworkstate.station_retry_num < 20)
        {
            esp_wifi_connect();
            wifinetworkstate.station_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            //WIFI连接失败

        }
        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifinetworkstate.station_retry_num = 0;
        wifinetworkstate.station_is_connect_ap=true;
    }
}

static void wifinetwork_sta_init()
{

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_sta_event_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &wifi_sta_event_handler,
                    NULL,
                    NULL));
    wifi_config_t wifi_config =
    {
        .sta = {
            .ssid = CONFIG_WIFI_NETWORK_ROUTER_AP_SSID,
            .password = CONFIG_WIFI_NETWORK_ROUTER_AP_PASSWORD,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            //.threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    {//加载文件配置
        cJSON *obj=system_config_get_item("wifinetwork");
        if(obj==NULL)
        {
            wifinetwork_station_set_config(NULL,NULL);
        }
        else
        {
            if(cJSON_HasObjectItem(obj,"station_ssid") && cJSON_HasObjectItem(obj,"station_password"))
            {
                cJSON *ssid=cJSON_GetObjectItem(obj,"station_ssid");
                cJSON *password=cJSON_GetObjectItem(obj,"station_password");
                if(cJSON_IsString(ssid))
                {
                    memset(wifi_config.sta.ssid,0,sizeof(wifi_config.sta.ssid));
                    memcpy(wifi_config.sta.ssid,
                           cJSON_GetStringValue(ssid),
                           sizeof(wifi_config.sta.ssid)>strlen(cJSON_GetStringValue(ssid))?strlen(cJSON_GetStringValue(ssid)):sizeof(wifi_config.sta.ssid));
                }

                if(cJSON_IsString(password))
                {
                    memset(wifi_config.sta.password,0,sizeof(wifi_config.sta.password));
                    memcpy(wifi_config.sta.password,
                           cJSON_GetStringValue(password),
                           sizeof(wifi_config.sta.password)>strlen(cJSON_GetStringValue(password))?strlen(cJSON_GetStringValue(password)):sizeof(wifi_config.sta.password));
                }
            }
            else
            {
                wifinetwork_station_set_config(NULL,NULL);
            }

            cJSON_Delete(obj);
        }
    }

    if(strlen((char *)wifi_config.sta.ssid)!=0)
    {
        wifinetworkstate.station_is_enable=true;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    }
}

#endif // CONFIG_WIFI_NETWORK_SOFTAP
#ifndef CONFIG_WIFI_NETWORK_STA

static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifinetworkstate.ap_station_count++;
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifinetworkstate.ap_station_count--;
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}



static void wifinetwork_ap_init()
{

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_ap_event_handler,
                    NULL,
                    NULL));
    wifi_config_t wifi_config =
    {
        .ap = {
            .ssid = CONFIG_WIFI_NETWORK_AP_SSID,
            .ssid_len = strlen(CONFIG_WIFI_NETWORK_AP_SSID),
            .channel = 7,
            .password = CONFIG_WIFI_NETWORK_AP_PASSWORD,
            .max_connection = 10,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

     {//加载文件配置
        cJSON *obj=system_config_get_item("wifinetwork");
        if(obj==NULL)
        {
            wifinetwork_ap_set_config(NULL,NULL);
        }
        else
        {
            if(cJSON_HasObjectItem(obj,"ap_ssid") && cJSON_HasObjectItem(obj,"ap_password"))
            {
                cJSON *ssid=cJSON_GetObjectItem(obj,"ap_ssid");
                cJSON *password=cJSON_GetObjectItem(obj,"ap_password");
                if(cJSON_IsString(ssid))
                {
                    memset(wifi_config.ap.ssid,0,sizeof(wifi_config.ap.ssid));
                    memcpy(wifi_config.ap.ssid,
                           cJSON_GetStringValue(ssid),
                           sizeof(wifi_config.ap.ssid)>strlen(cJSON_GetStringValue(ssid))?strlen(cJSON_GetStringValue(ssid)):sizeof(wifi_config.ap.ssid));
                }

                if(cJSON_IsString(password))
                {
                    memset(wifi_config.ap.password,0,sizeof(wifi_config.ap.password));
                    memcpy(wifi_config.ap.password,
                           cJSON_GetStringValue(password),
                           sizeof(wifi_config.ap.password)>strlen(cJSON_GetStringValue(password))?strlen(cJSON_GetStringValue(password)):sizeof(wifi_config.ap.password));
                }
            }
            else
            {
                wifinetwork_ap_set_config(NULL,NULL);
            }

            cJSON_Delete(obj);
        }
    }

    if (strlen((char *)wifi_config.ap.password) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    if(strlen((char *)wifi_config.ap.ssid) != 0 )
    {
        wifinetworkstate.ap_is_enable=true;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    }

}
#endif // CONFIG_WIFI_NETWORK_STA
//初始化wifinetwork
void wifinetwork_init()
{
#if CONFIG_WIFI_NETWORK == 1




#ifndef CONFIG_WIFI_NETWORK_STA
    esp_netif_create_default_wifi_ap();
    wifinetworkstate.ap_is_enable=false;
#endif // CONFIG_WIFI_NETWORK_STA
#ifndef CONFIG_WIFI_NETWORK_SOFTAP
    esp_netif_create_default_wifi_sta();
    wifinetworkstate.station_is_enable=false;
#endif // CONFIG_WIFI_NETWORK_SOFTAP

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

#ifdef CONFIG_WIFI_NETWORK_STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifinetwork_sta_init();
#endif // CONFIG_WIFI_NETWORK_STA

#ifdef CONFIG_WIFI_NETWORK_SOFTAP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifinetwork_ap_init();
#endif // CONFIG_WIFI_NETWORK_SOFTAP

#ifdef CONFIG_WIFI_NETWORK_SOFTAPSTA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    wifinetwork_ap_init();
    wifinetwork_sta_init();
#endif // CONFIG_WIFI_NETWORK_SOFTAPSTA

    //启动WIFI
    ESP_ERROR_CHECK(esp_wifi_start());

#endif // CONFIG_WIFI_NETWORK
}

wifinetwork_state_t wifinetwork_getstate()
{
    return wifinetworkstate;
}

#ifndef CONFIG_WIFI_NETWORK_SOFTAP
//设置STA相关参数
void wifinetwork_station_set_config(const char * ssid,const char * password)
{
    cJSON *obj=system_config_get_item("wifinetwork");
    if(obj==NULL)
    {
        obj=cJSON_CreateObject();
    }

    {//删除原有相关项
        if(cJSON_HasObjectItem(obj,"station_ssid"))
        {
            cJSON_DeleteItemFromObject(obj,"station_ssid");
        }
        if(cJSON_HasObjectItem(obj,"station_password"))
        {
            cJSON_DeleteItemFromObject(obj,"station_password");
        }
    }

    {//写入新数据
        if(ssid==NULL)
        {
            cJSON *tempobj=cJSON_CreateString(CONFIG_WIFI_NETWORK_ROUTER_AP_SSID);
            cJSON_AddItemToObject(obj,"station_ssid",tempobj);
        }
        else
        {
            cJSON *tempobj=cJSON_CreateString(ssid);
            cJSON_AddItemToObject(obj,"station_ssid",tempobj);
        }

        if(password==NULL)
        {
            cJSON *tempobj=cJSON_CreateString(CONFIG_WIFI_NETWORK_ROUTER_AP_PASSWORD);
            cJSON_AddItemToObject(obj,"station_password",tempobj);
        }
        else
        {
            cJSON *tempobj=cJSON_CreateString(password);
            cJSON_AddItemToObject(obj,"station_password",tempobj);
        }
    }

    //保存数据
    system_config_put_item(obj,"wifinetwork");
    system_config_save();

    cJSON_Delete(obj);
}
#endif // CONFIG_WIFI_NETWORK_SOFTAP

#ifndef CONFIG_WIFI_NETWORK_STA
//设置AP相关参数
void wifinetwork_ap_set_config(const char * ssid,const char * password)
{
    cJSON *obj=system_config_get_item("wifinetwork");
    if(obj==NULL)
    {
        obj=cJSON_CreateObject();
    }

    {//删除原有相关项
        if(cJSON_HasObjectItem(obj,"ap_ssid"))
        {
            cJSON_DeleteItemFromObject(obj,"ap_ssid");
        }
        if(cJSON_HasObjectItem(obj,"ap_password"))
        {
            cJSON_DeleteItemFromObject(obj,"ap_password");
        }
    }

    {//写入新数据
        if(ssid==NULL)
        {
            cJSON *tempobj=cJSON_CreateString(CONFIG_WIFI_NETWORK_AP_SSID);
            cJSON_AddItemToObject(obj,"ap_ssid",tempobj);
        }
        else
        {
            cJSON *tempobj=cJSON_CreateString(ssid);
            cJSON_AddItemToObject(obj,"ap_ssid",tempobj);
        }

        if(password==NULL)
        {
            cJSON *tempobj=cJSON_CreateString(CONFIG_WIFI_NETWORK_AP_PASSWORD);
            cJSON_AddItemToObject(obj,"ap_password",tempobj);
        }
        else
        {
            cJSON *tempobj=cJSON_CreateString(password);
            cJSON_AddItemToObject(obj,"ap_password",tempobj);
        }
    }

    //保存数据
    system_config_put_item(obj,"wifinetwork");
    system_config_save();

    cJSON_Delete(obj);
}
#endif // CONFIG_WIFI_NETWORK_STA
