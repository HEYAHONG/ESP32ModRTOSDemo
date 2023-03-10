
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "sdkconfig.h"
#include "appconfig.h"
#include "init.h"
#include "wifinetwork.h"
#include "wifimeshnet.h"
#include "ethernet.h"
#include "tftpd.h"
#include "mqtt.h"
#include "app.h"
#include "RC.h"
#include <inttypes.h>

static const char *TAG = "esp32 main";

#if CONFIG_WIFI_MESH_NETWORK == 1
static wifimeshnet_callback_t meshcb = {NULL, NULL, NULL};
#endif // CONFIG_WIFI_MESH_NETWORK

#if CONFIG_ETHERNET_NETWORK == 1
static ethernet_network_callback_t ethcb = {NULL, NULL};
#endif // CONFIG_ETHERNET_NETWORK

static void main_task()
{

    system_init();

#if CONFIG_ETHERNET_NETWORK == 1
    ethernet_network_init(ethcb);
#endif // CONFIG_ETHERNET_NETWORK

#if CONFIG_WIFI_NETWORK == 1
    wifinetwork_init();
#endif // CONFIG_WIFI_NETWORK

#if CONFIG_WIFI_MESH_NETWORK == 1
    wifimeshnet_init(meshcb);
#endif // CONFIG_WIFI_MESH_NETWORK

#if CONFIG_LWIP_TFTPD_ON_BOOT == 1
    tftpd_start();
#endif // LWIP_TFTPD_ON_BOOT


    app_init();

    ESP_LOGI(TAG, "FreeMemory:%" PRIu32 " Bytes,Min FreeMemory:%" PRIu32 " Bytes ", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

    while (1)
    {
        app_loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}

void app_main(void)
{
    xTaskCreate(main_task, "main_task", 4096, NULL, 10, NULL);
}
