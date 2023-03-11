#include "appconfig.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "app.h"
#include "libSMGS.h"
#include <inttypes.h>

#ifdef CONFIG_MQTT_CLIENT_USE_SMGS
void SMGS_Init();
#endif // CONFIG_MQTT_CLIENT_USE_SMGS

#ifdef CONFIG_MQTT_CLIENT_USE_ONENET_DEVICE
void OneNETDevice_Init();
#endif // CONFIG_MQTT_CLIENT_USE_ONENET_DEVICE

static const char *TAG = "App";

static uint8_t mac[6] = {0};
char macstr[20] = {0};
void app_init()
{
    ESP_LOGI(TAG, "Started!\r\n");

    {
        //获取mac地址用于唯一标识
        bool IsFailed = false;
        if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK)
        {
            ESP_LOGI(TAG, "Get Mac Failed!Try WIFI Init!\r\n");
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));
            if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK)
            {
                IsFailed = true;
                ESP_LOGI(TAG, "Get Mac Failed!\r\n");
                ESP_ERROR_CHECK(esp_wifi_deinit());
            }

        }

        if (!IsFailed)
        {
            for (size_t i = 0; i < sizeof(mac); i++)
            {
                char buff[6] = {0};
                sprintf(buff, "%02X", mac[i]);
                strcat(macstr, buff);
            }

            ESP_LOGI(TAG, "Mac Is %s!\r\n", macstr);

        }
    }

#ifdef CONFIG_MQTT_CLIENT_USE_SMGS
    SMGS_Init();
#endif // CONFIG_MQTT_CLIENT_USE_SMGS

#ifdef CONFIG_MQTT_CLIENT_USE_ONENET_DEVICE
    OneNETDevice_Init();
#endif // CONFIG_MQTT_CLIENT_USE_ONENET_DEVICE

}

void app_loop()
{
    vTaskDelay(1);
}


