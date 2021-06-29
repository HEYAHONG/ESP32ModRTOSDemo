
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


static const char *TAG = "esp32 main";

static void main_task ()
{

    system_init();



    ESP_LOGI(TAG,"FreeMemory:%d Bytes",esp_get_minimum_free_heap_size());

    while (1)
    {
        vTaskDelay (2000 / portTICK_PERIOD_MS);

    }

}

void app_main (void)
{
    xTaskCreate (main_task, "main_task", 4096, NULL, 10, NULL);
}
