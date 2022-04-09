#include "wifinetwork.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"
#include "stdbool.h"
#include "init.h"
#include "esp_smartconfig.h"



static volatile wifinetwork_state_t wifinetworkstate= {0};


#if CONFIG_WIFI_NETWORK == 1

static const char *TAG = "wifi network";

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
#ifdef CONFIG_WIFI_NETWORK_SMARTCONFIG
        if (wifinetworkstate.station_retry_num < CONFIG_WIFI_NETWORK_RETRY_COUNT)
        {
            esp_wifi_connect();
            wifinetworkstate.station_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            //WIFI连接失败
            if(wifinetworkstate.station_is_connect_ap_ever)
            {
                esp_wifi_connect();
                wifinetworkstate.station_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            }
            else
            {
                wifinetworkstate.station_retry_num=0;
                wifinetwork_sta_smartconfg_start(CONFIG_ESP_SMARTCONFIG_TYPE,CONFIG_WIFI_NETWORK_SMARTCONFIG_TIMEOUT);
            }

        }
        ESP_LOGI(TAG,"connect to the AP fail");
#else
        esp_wifi_connect();
        wifinetworkstate.station_retry_num++;
        ESP_LOGI(TAG, "retry to connect to the AP");
#endif // CONFIG_WIFI_NETWORK_SMARTCONFIG
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifinetworkstate.station_retry_num = 0;
        wifinetworkstate.station_is_connect_ap=true;
        wifinetworkstate.station_is_connect_ap_ever=true;
    }
}

static esp_event_handler_instance_t  wifinetwork_sta_event_wifi_handler=NULL;
static esp_event_handler_instance_t  wifinetwork_sta_event_ip_handler=NULL;

static void wifinetwork_sta_deinit()
{
    if(wifinetwork_sta_event_wifi_handler!=NULL)
    {

        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,
                        ESP_EVENT_ANY_ID,
                        wifinetwork_sta_event_wifi_handler));
        wifinetwork_sta_event_wifi_handler=NULL;
    }
    if(wifinetwork_sta_event_ip_handler!=NULL)
    {

        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,
                        IP_EVENT_STA_GOT_IP,
                        wifinetwork_sta_event_ip_handler));
        wifinetwork_sta_event_ip_handler=NULL;
    }
}

static void wifinetwork_sta_init()
{

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_sta_event_handler,
                    NULL,
                    &wifinetwork_sta_event_wifi_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &wifi_sta_event_handler,
                    NULL,
                    &wifinetwork_sta_event_ip_handler));
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

    {
        //加载文件配置
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
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
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

static esp_event_handler_instance_t  wifinetwork_ap_event_wifi_handler=NULL;

static void wifinetwork_ap_deinit()
{
    if(wifinetwork_ap_event_wifi_handler!=NULL)
    {

        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,
                        ESP_EVENT_ANY_ID,
                        wifinetwork_ap_event_wifi_handler));
        wifinetwork_ap_event_wifi_handler=NULL;
    }
}

static void wifinetwork_ap_init()
{

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_ap_event_handler,
                    NULL,
                    &wifinetwork_ap_event_wifi_handler));
    wifi_config_t wifi_config =
    {
        .ap = {
            .ssid = CONFIG_WIFI_NETWORK_AP_SSID,
            .ssid_len = strlen(CONFIG_WIFI_NETWORK_AP_SSID),
            .channel = CONFIG_WIFI_NETWORK_AP_CHANNEL,
            .password = CONFIG_WIFI_NETWORK_AP_PASSWORD,
            .max_connection = CONFIG_WIFI_NETWORK_AP_MAX_CONNECTION,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    {
        //加载文件配置
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
                    wifi_config.ap.ssid_len=strlen((char *)wifi_config.ap.ssid);
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
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

}
#endif // CONFIG_WIFI_NETWORK_STA

#endif // CONFIG_WIFI_NETWORK

//初始化wifinetwork
void wifinetwork_init()
{
    wifinetworkstate.wifinetwork_running=false;

#if CONFIG_WIFI_NETWORK == 1



#ifndef CONFIG_WIFI_NETWORK_STA
    esp_netif_create_default_wifi_ap();
    wifinetworkstate.ap_is_enable=false;
#endif // CONFIG_WIFI_NETWORK_STA
#ifndef CONFIG_WIFI_NETWORK_SOFTAP
    esp_netif_create_default_wifi_sta();
    wifinetworkstate.station_is_enable=false;
    wifinetworkstate.station_is_in_smartconfig=false;
    wifinetworkstate.station_is_connect_ap_ever=false;
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

    wifinetworkstate.wifinetwork_running=true;

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
#if CONFIG_WIFI_NETWORK == 1
    cJSON *obj=system_config_get_item("wifinetwork");
    if(obj==NULL)
    {
        obj=cJSON_CreateObject();
    }

    {
        //删除原有相关项
        if(cJSON_HasObjectItem(obj,"station_ssid"))
        {
            cJSON_DeleteItemFromObject(obj,"station_ssid");
        }
        if(cJSON_HasObjectItem(obj,"station_password"))
        {
            cJSON_DeleteItemFromObject(obj,"station_password");
        }
    }

    {
        //写入新数据
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
#endif // CONFIG_WIFI_NETWORK
}
#endif // CONFIG_WIFI_NETWORK_SOFTAP

#ifndef CONFIG_WIFI_NETWORK_STA
//设置AP相关参数
void wifinetwork_ap_set_config(const char * ssid,const char * password)
{
#if CONFIG_WIFI_NETWORK == 1
    cJSON *obj=system_config_get_item("wifinetwork");
    if(obj==NULL)
    {
        obj=cJSON_CreateObject();
    }

    {
        //删除原有相关项
        if(cJSON_HasObjectItem(obj,"ap_ssid"))
        {
            cJSON_DeleteItemFromObject(obj,"ap_ssid");
        }
        if(cJSON_HasObjectItem(obj,"ap_password"))
        {
            cJSON_DeleteItemFromObject(obj,"ap_password");
        }
    }

    {
        //写入新数据
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
#endif // CONFIG_WIFI_NETWORK
}
#endif // CONFIG_WIFI_NETWORK_STA

//停止wifinetowrk
void wifinetwork_stop()
{
#if CONFIG_WIFI_NETWORK == 1
#ifdef CONFIG_WIFI_NETWORK_STA
    wifinetwork_sta_deinit();
#endif // CONFIG_WIFI_NETWORK_STA

#ifdef CONFIG_WIFI_NETWORK_SOFTAP
    wifinetwork_ap_deinit();
#endif // CONFIG_WIFI_NETWORK_SOFTAP

#ifdef CONFIG_WIFI_NETWORK_SOFTAPSTA
    wifinetwork_ap_deinit();
    wifinetwork_sta_deinit();
#endif // CONFIG_WIFI_NETWORK_SOFTAPSTA

    //停止WIFI
    ESP_ERROR_CHECK(esp_wifi_stop());
#endif // CONFIG_WIFI_NETWORK

    wifinetworkstate.wifinetwork_running=false;
}

//启动wifinetowrk
void wifinetwork_start()
{
#if CONFIG_WIFI_NETWORK == 1
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
    wifinetworkstate.wifinetwork_running=true;
}



#ifndef CONFIG_WIFI_NETWORK_SOFTAP

#if CONFIG_WIFI_NETWORK == 1

static EventGroupHandle_t wifinetwork_smartconfig_event_group=NULL;
/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

static TickType_t wifinetwork_smartconfig_stoptick=0;

static void smartconfig_check_task(void * parm)
{
    EventBits_t uxBits;
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(wifinetwork_smartconfig_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, pdMS_TO_TICKS(100));
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();

            //停止smartconfig
            wifinetwork_sta_smartconfg_stop();


            vTaskDelete(NULL);
        }

        TickType_t current_tick=xTaskGetTickCount();
        if((wifinetwork_smartconfig_stoptick-current_tick)>0 && (wifinetwork_smartconfig_stoptick-current_tick) <1000)
        {
            ESP_LOGI(TAG, "smartconfig timeout");
            esp_smartconfig_stop();

            //停止smartconfig
            wifinetwork_sta_smartconfg_stop();


            vTaskDelete(NULL);
        }
    }
}

static TaskHandle_t wifinetwork_smartconfig_check_task_handle=NULL;

static void wifinetwork_smartconfig_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
       xTaskCreate(smartconfig_check_task, "smartconfig_check_task", 4096, NULL, 3, &wifinetwork_smartconfig_check_task_handle);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits( wifinetwork_smartconfig_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits( wifinetwork_smartconfig_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(TAG, "RVD_DATA:");
            for (int i=0; i<33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );


        //保存密码
        wifinetwork_station_set_config((char *)ssid,(char *)password);



        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(wifinetwork_smartconfig_event_group, ESPTOUCH_DONE_BIT);
    }
}


static esp_event_handler_instance_t  wifinetwork_sta_smartconfig_event_wifi_handler=NULL;
static esp_event_handler_instance_t  wifinetwork_sta_smartconfig_event_ip_handler=NULL;
static esp_event_handler_instance_t  wifinetwork_sta_smartconfig_event_sc_handler=NULL;

#endif // CONFIG_WIFI_NETWORK

//启动smartconfig
void wifinetwork_sta_smartconfg_start(smartconfig_type_t sctype,size_t timeout_s)
{

#if CONFIG_WIFI_NETWORK == 1

    if(wifinetworkstate.station_is_in_smartconfig==true)
    {
        return;
    }

    wifinetwork_smartconfig_stoptick=xTaskGetTickCount()+timeout_s*1000*pdMS_TO_TICKS(timeout_s);

    wifinetwork_stop();

    if(wifinetwork_smartconfig_event_group==NULL)
    {
        wifinetwork_smartconfig_event_group = xEventGroupCreate();
    }

    xEventGroupClearBits(wifinetwork_smartconfig_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT);

    ESP_ERROR_CHECK( esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifinetwork_smartconfig_event_handler, NULL,&wifinetwork_sta_smartconfig_event_wifi_handler) );
    ESP_ERROR_CHECK( esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifinetwork_smartconfig_event_handler, NULL,&wifinetwork_sta_smartconfig_event_ip_handler) );
    ESP_ERROR_CHECK( esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifinetwork_smartconfig_event_handler, NULL, &wifinetwork_sta_smartconfig_event_sc_handler));

    ESP_ERROR_CHECK( esp_smartconfig_set_type(sctype) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
#endif // CONFIG_WIFI_NETWORK
}

//停止smartconfig
void wifinetwork_sta_smartconfg_stop()
{
#if CONFIG_WIFI_NETWORK == 1
    if(wifinetworkstate.station_is_in_smartconfig==true)
    {
        vTaskDelete(wifinetwork_smartconfig_check_task_handle);
    }

    if(wifinetwork_sta_smartconfig_event_wifi_handler !=NULL)
    {
        ESP_ERROR_CHECK( esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifinetwork_sta_smartconfig_event_wifi_handler) );
        wifinetwork_sta_smartconfig_event_wifi_handler=NULL;
    }
    if(wifinetwork_sta_smartconfig_event_ip_handler!=NULL)
    {
        ESP_ERROR_CHECK( esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifinetwork_sta_smartconfig_event_ip_handler) );
        wifinetwork_sta_smartconfig_event_ip_handler=NULL;
    }
    if(wifinetwork_sta_smartconfig_event_sc_handler!=NULL)
    {
        ESP_ERROR_CHECK( esp_event_handler_instance_unregister(SC_EVENT, ESP_EVENT_ANY_ID,wifinetwork_sta_smartconfig_event_sc_handler ));
        wifinetwork_sta_smartconfig_event_sc_handler=NULL;
    }

    //停止WIFI网络
    wifinetwork_stop();

    wifinetwork_smartconfig_stoptick=0;
    wifinetworkstate.station_is_in_smartconfig=false;

    //启动wifi网络
    wifinetwork_start();
#endif // CONFIG_WIFI_NETWORK
}


#endif // CONFIG_WIFI_NETWORK_SOFTAP
