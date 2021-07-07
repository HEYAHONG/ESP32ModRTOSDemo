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


//启动mqtt
void mqttc_start();

//停止mqtt
void mqttc_stop();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CONFIG_NETWORK_PROTOCAL_MQTT

#endif // MQTT_H_INCLUDED
