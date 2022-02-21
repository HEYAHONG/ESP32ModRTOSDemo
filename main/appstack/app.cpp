#include "appconfig.h"
#include "esp_log.h"
#include "app.h"

static const char *TAG="App";

void app_init()
{
    ESP_LOGI(TAG,"Started!\r\n");
}

void app_loop()
{
    vTaskDelay(1);
}
