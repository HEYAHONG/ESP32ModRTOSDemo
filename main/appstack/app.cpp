#include "appconfig.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "app.h"
#include "libSMGS.h"
#include <inttypes.h>

#ifdef CONFIG_MQTT_CLIENT_USE_SMGS
static void SMGS_Init();
#endif // CONFIG_MQTT_CLIENT_USE_SMGS

static const char *TAG="App";

static uint8_t mac[6]= {0};
char macstr[20]= {0};
void app_init()
{
    ESP_LOGI(TAG,"Started!\r\n");

    {
        //获取mac地址用于唯一标识
        bool IsFailed=false;
        if(esp_wifi_get_mac(WIFI_IF_STA,mac)!=ESP_OK)
        {
            ESP_LOGI(TAG,"Get Mac Failed!Try WIFI Init!\r\n");
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));
            if(esp_wifi_get_mac(WIFI_IF_STA,mac)!=ESP_OK)
            {
                IsFailed=true;
                ESP_LOGI(TAG,"Get Mac Failed!\r\n");
                ESP_ERROR_CHECK(esp_wifi_deinit());
            }

        }

        if(!IsFailed)
        {
            for(size_t i=0; i<sizeof(mac); i++)
            {
                char buff[6]= {0};
                sprintf(buff,"%02X",mac[i]);
                strcat(macstr,buff);
            }

            ESP_LOGI(TAG,"Mac Is %s!\r\n",macstr);

        }
    }

#ifdef CONFIG_MQTT_CLIENT_USE_SMGS
    SMGS_Init();
#endif // CONFIG_MQTT_CLIENT_USE_SMGS

}

void app_loop()
{
    vTaskDelay(1);
}

#ifdef CONFIG_MQTT_CLIENT_USE_SMGS

#include "mqtt.h"
/*
协议栈相关
*/
const char *GateWayName="ESP32Demo";
char GateWaySerialNumber[32]="ES32";
SMGS_device_context_t device_context;

bool SMGS_IsOnline(struct __SMGS_device_context_t *ctx)
{
    //默认返回真
    return true;
}

bool SMGS_Device_Command(SMGS_device_context_t *ctx,SMGS_topic_string_ptr_t plies[],SMGS_payload_cmdid_t *cmdid,uint8_t *cmddata,size_t cmddata_length,uint8_t *retbuff,size_t *retbuff_length,SMGS_payload_retcode_t *ret)
{
    bool _ret=false;
    ESP_LOGI(TAG,"Device_Command(CmdID=%04" PRIX32 ")\r\n",(uint32_t)(*cmdid));
    return _ret;
}

bool SMGS_Device_ReadRegister(SMGS_device_context_t *ctx,SMGS_topic_string_ptr_t plies[],SMGS_payload_register_address_t addr,uint64_t *dat,SMGS_payload_register_flag_t *flag)
{
    bool ret=false;
    ESP_LOGI(TAG,"Device_ReadRegister(Addr=%04" PRIX32 ")\r\n",(uint32_t)addr);
    return ret;
}

bool SMGS_Device_WriteRegister(SMGS_device_context_t *ctx,SMGS_topic_string_ptr_t plies[],SMGS_payload_register_address_t addr,uint64_t *dat,SMGS_payload_register_flag_t *flag)
{
    bool ret=false;
    ESP_LOGI(TAG,"Device_WriteRegister(Addr=%04" PRIX32 ",Data=%016" PRIX64 ",Flag=%02" PRIX32 ")\r\n",(uint32_t)addr,(*dat),(uint32_t)(flag->val));
    return ret;
}

bool SMGS_Device_ReadSensor(SMGS_device_context_t *ctx,SMGS_topic_string_ptr_t plies[],SMGS_payload_sensor_address_t addr,uint64_t *dat,SMGS_payload_sensor_flag_t *flag)
{
    bool ret=false;
    ESP_LOGI(TAG,"Device_ReadSensor(Addr=%04" PRIX32 ",Flag=%02" PRIX32 ")\r\n",(uint32_t)addr,(uint32_t)(flag->val));
    return ret;
}


SMGS_gateway_context_t gateway_context;

bool SMGS_GateWay_Command(SMGS_gateway_context_t *ctx,SMGS_topic_string_ptr_t plies[],SMGS_payload_cmdid_t *cmdid,uint8_t *cmddata,size_t cmddata_length,uint8_t *retbuff,size_t *retbuff_length,SMGS_payload_retcode_t *ret)
{
    bool _ret=false;
    ESP_LOGI(TAG,"GateWay_Command(CmdID=%04" PRIX32 ")\r\n",(uint32_t)(*cmdid));
    return _ret;
}

bool SMGS_GateWay_ReadRegister(SMGS_gateway_context_t *ctx,SMGS_topic_string_ptr_t plies[],SMGS_payload_register_address_t addr,uint64_t *dat,SMGS_payload_register_flag_t *flag)
{
    bool ret=false;
    ESP_LOGI(TAG,"GateWay_ReadRegister(Addr=%04" PRIX32 ")\r\n",(uint32_t)addr);
    return ret;
}

bool SMGS_GateWay_WriteRegister(SMGS_gateway_context_t *ctx,SMGS_topic_string_ptr_t plies[],SMGS_payload_register_address_t addr,uint64_t *dat,SMGS_payload_register_flag_t *flag)
{
    bool ret=false;
    ESP_LOGI(TAG,"GateWay_WriteRegister(Addr=%04" PRIX32 ",Data=%016" PRIX64 ",Flag=%02" PRIX32 ")\r\n",(uint32_t)addr,(*dat),(uint32_t)(flag->val));
    return ret;
}

bool SMGS_GateWay_ReadSensor(SMGS_gateway_context_t *ctx,SMGS_topic_string_ptr_t plies[],SMGS_payload_sensor_address_t addr,uint64_t *dat,SMGS_payload_sensor_flag_t *flag)
{
    bool ret=false;
    ESP_LOGI(TAG,"GateWay_ReadSensor(Addr=%04" PRIX32 ",Flag=%02" PRIX32 ")\r\n",(uint32_t)addr,(uint32_t)(flag->val));
    return ret;
}



//设备查询函数
SMGS_device_context_t * SMGS_Device_Next(struct __SMGS_gateway_context_t *ctx,SMGS_device_context_t * devctx)
{
    if(devctx==NULL)
    {
        return &device_context;//返回第一个设备
    }

    //由于只有一个设备，直接返回NULL

    return NULL;
}


static bool SMGS_MessagePublish(struct __SMGS_gateway_context_t *ctx,const char * topic,void * payload,size_t payloadlen,uint8_t qos,int retain)
{
    return mqttc_publish(topic,(const char *)payload,payloadlen,qos,retain);
}

static bool mqttc_event_callback(esp_mqtt_event_id_t event_id,esp_mqtt_event_handle_t event);//返回true表示消息已处理，无需其它处理
static void mqttc_event_on_init_config(esp_mqtt_client_config_t *mqtt_cfg);

//SMGS初始化
static void SMGS_Init()
{
    strcat(GateWaySerialNumber,macstr);

    {
        //初始化设备上下文
        SMGS_Device_Context_Init(&device_context);

        //填写设备上下文
        device_context.DeviceName=GateWayName;
        device_context.DevicePosNumber=1;
        device_context.DeviceSerialNumber=GateWaySerialNumber;//默认序列号同网关
        device_context.IsOnline=SMGS_IsOnline;
        device_context.Command=SMGS_Device_Command;
        device_context.ReadRegister=SMGS_Device_ReadRegister;
        device_context.WriteRegister=SMGS_Device_WriteRegister;
        device_context.ReadSensor=SMGS_Device_ReadSensor;

    }

    {

        //初始化网关上下文
        SMGS_GateWay_Context_Init(&gateway_context,GateWaySerialNumber,SMGS_MessagePublish);

        //填写网关上下文
        gateway_context.GateWayName=GateWayName;
        gateway_context.Command=SMGS_GateWay_Command;
        gateway_context.ReadRegister=SMGS_GateWay_ReadRegister;
        gateway_context.WriteRegister=SMGS_GateWay_WriteRegister;
        gateway_context.ReadSensor=SMGS_GateWay_ReadSensor;
        gateway_context.Device_Next=SMGS_Device_Next;
    }

    //启动mqttc
    mqttc_start(mqttc_event_on_init_config,mqttc_event_callback);

}


static bool mqttc_event_callback(esp_mqtt_event_id_t event_id,esp_mqtt_event_handle_t event)//返回true表示消息已处理，无需其它处理
{
    switch(event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        //发送网关上线消息
        uint8_t buff[256]= {0};
        SMGS_GateWay_Online(&gateway_context,buff,sizeof(buff),0,0);
    }
    {
        char subscribestr[64]= {0};
        strcat(subscribestr,GateWaySerialNumber);
        strcat(subscribestr,"/#");
        esp_mqtt_client_subscribe(event->client,subscribestr,0);

    }
    break;

    case MQTT_EVENT_DATA:
    {
        const size_t buffsize=4096;
        uint8_t *buff=(uint8_t *)malloc(buffsize);
        SMGS_GateWay_Receive_MQTT_MSG(&gateway_context,event->topic,event->topic_len,(uint8_t *)event->data,event->data_len,event->qos,event->retain?1:0,buff,buffsize);
        free(buff);
        return true;//返回真,后续不处理
    }
    break;


    default:
        break;
    }
    return false;
}

static uint8_t willbuff[256]= {0};
static SMGS_gateway_will_t will;
static void mqttc_event_on_init_config(esp_mqtt_client_config_t *mqtt_cfg)
{
    //填写will

    memset(&will,0,sizeof(will));
    SMGS_GateWay_Will_Encode(&gateway_context,&will,willbuff,sizeof(willbuff));

    mqtt_cfg->session.protocol_ver=MQTT_PROTOCOL_V_3_1_1;

    mqtt_cfg->session.last_will.topic=will.topic;
    mqtt_cfg->session.last_will.msg=(const char *)will.payload;
    mqtt_cfg->session.last_will.msg_len=will.payloadlen;
    mqtt_cfg->session.last_will.qos=will.qos;
    mqtt_cfg->session.last_will.retain=will.ratain;


    //keepalive
    mqtt_cfg->session.keepalive=120;

    //clientid、用户名、密码
    mqtt_cfg->credentials.client_id=gateway_context.GateWaySerialNumber;
    mqtt_cfg->credentials.username=gateway_context.GateWaySerialNumber;
    mqtt_cfg->credentials.authentication.password=gateway_context.GateWaySerialNumber;

}

#endif // CONFIG_MQTT_CLIENT_USE_SMGS

