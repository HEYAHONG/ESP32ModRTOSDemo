#ifndef MQTT_H_INCLUDED
#define MQTT_H_INCLUDED

#include "appconfig.h"
#include "mqtt_client.h"
#include "esp_log.h"

#if CONFIG_NETWORK_PROTOCAL_MQTT == 1

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef bool (*mqttc_event_callback_t)(esp_mqtt_event_id_t event_id, esp_mqtt_event_handle_t event); //返回true表示消息已处理，无需其它处理
typedef void (*mqttc_event_on_init_config_t)(esp_mqtt_client_config_t *mqtt_cfg);


//启动mqtt
void mqttc_start(mqttc_event_on_init_config_t on_cfg, mqttc_event_callback_t cb);

//停止mqtt
void mqttc_stop();


//publish mqtt消息(包装库函数)
bool mqttc_publish(const char *topic, const char *data, int len, int qos, int retain);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CONFIG_NETWORK_PROTOCAL_MQTT

#endif // MQTT_H_INCLUDED
