#include "appconfig.h"
#include "init.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "tftpd.h"
#include "cJSON.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"



static const char *TAG = "esp32 init";

//初始化文件系统用于文件存储
static void init_spiffs()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf =
    {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 100,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}



//全局配置文件json
static cJSON * golbalconfig=NULL;
static SemaphoreHandle_t golbalconfig_mutex=NULL;

void init_json()
{
    {
        //采用FreeRTOS的分配函数(方便修改)
        cJSON_Hooks cjson_hook=
        {
            pvPortMalloc,
            vPortFree
        };
        cJSON_InitHooks(&cjson_hook);
    }
    {
        //加载文件内容
        FILE * fp=fopen("/spiffs/"CONFIG_GOLBAL_CONFIG_FILENAME,"r");
        if(fp!=NULL)
        {
            fseek(fp,0,SEEK_END);

            size_t length=ftell(fp);

            char *buff=pvPortMalloc(length+1);

            memset(buff,0,length+1);

            fseek(fp,0,SEEK_SET);

            fread(buff,1,length,fp);

            golbalconfig=cJSON_ParseWithLength(buff,length);

            vPortFree(buff);

            fclose(fp);
        }
        else
        {
            ESP_LOGI(TAG,"Load "CONFIG_GOLBAL_CONFIG_FILENAME" from spiffs failed!");
        }
    }

    if(golbalconfig==NULL)
    {
        golbalconfig=cJSON_Parse(DEFAULT_GOLBAL_CONFIG_JSON);
    }

    {
        //创建锁
        golbalconfig_mutex=xSemaphoreCreateMutex();

        xSemaphoreGive(golbalconfig_mutex);
    }
}
//初始化
void system_init()
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());


    init_spiffs();


    init_json();

#if CONFIG_LWIP_TFTPD_ON_BOOT == 1
    tftpd_start();
#endif // LWIP_TFTPD_ON_BOOT

}

void system_config_save()
{
    if(golbalconfig==NULL)
    {
        return;
    }

    FILE *fp=fopen("/spiffs/"CONFIG_GOLBAL_CONFIG_FILENAME,"w");
    if(fp!=NULL)
    {


        xSemaphoreTake(golbalconfig_mutex, portMAX_DELAY);

        char * buff=cJSON_Print(golbalconfig);


        xSemaphoreGive(golbalconfig_mutex);

        fwrite(buff,strlen(buff),1,fp);

        cJSON_free(buff);

        fclose(fp);
    }
}

void system_config_put_item(cJSON *item,const char * name)
{
    if(golbalconfig==NULL)
    {
        return;
    }

    if(item==NULL)
    {
        return ;
    }

    if(name == NULL || strlen(name)==0)
    {
        return;
    }


    xSemaphoreTake(golbalconfig_mutex, portMAX_DELAY);

    if(cJSON_HasObjectItem(golbalconfig,name))
    {
        cJSON_DeleteItemFromObject(golbalconfig,name);
    }

    cJSON *obj=cJSON_Duplicate(item,1);

    cJSON_AddItemToObject(golbalconfig,name,obj);

    xSemaphoreGive(golbalconfig_mutex);
}

cJSON * system_config_get_item(const char *name)
{

    if(golbalconfig==NULL)
    {
        return NULL;
    }

    if(name == NULL || strlen(name)==0)
    {
        return NULL;
    }


    xSemaphoreTake(golbalconfig_mutex, portMAX_DELAY);

    cJSON *ret=NULL;

    if(cJSON_HasObjectItem(golbalconfig,name))
    {
        ret= cJSON_Duplicate(cJSON_GetObjectItem(golbalconfig,name),1);
    }

    xSemaphoreGive(golbalconfig_mutex);

    return ret;

}
