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

static const char *TAG = "wifi network";


#ifndef CONFIG_WIFI_NETWORK_SOFTAP

static int s_retry_num = 0;

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < 20)
        {
            esp_wifi_connect();
            s_retry_num++;
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
        s_retry_num = 0;
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
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

}

#endif // CONFIG_WIFI_NETWORK_SOFTAP
#ifndef CONFIG_WIFI_NETWORK_STA

static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
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
    if (strlen(CONFIG_WIFI_NETWORK_AP_PASSWORD) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

}
#endif // CONFIG_WIFI_NETWORK_STA
//初始化wifinetwork
void wifinetwork_init()
{
#if CONFIG_WIFI_NETWORK == 1

#ifndef CONFIG_WIFI_NETWORK_STA
    esp_netif_create_default_wifi_ap();
#endif // CONFIG_WIFI_NETWORK_STA
#ifndef CONFIG_WIFI_NETWORK_SOFTAP
    esp_netif_create_default_wifi_sta();
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
