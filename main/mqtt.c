#include "mqtt.h"

#if CONFIG_NETWORK_PROTOCAL_MQTT == 1


static const char *TAG = "MQTT Client";

mqttc_event_callback_t callback=NULL;
mqttc_event_on_init_config_t on_config=NULL;

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    if(callback!=NULL)
    {
        if(callback(event_id,event))
        {
            return;//已处理直接返回
        }
    }
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}



static esp_mqtt_client_handle_t client=NULL;


//启动mqtt
void mqttc_start(mqttc_event_on_init_config_t on_cfg,mqttc_event_callback_t cb)
{

    if(client!=NULL)
    {
        mqttc_stop();
    }

    on_config=on_cfg;
    callback=cb;


    esp_mqtt_client_config_t mqtt_cfg =
    {
        .uri = CONFIG_BROKER_URL,
    };

    if(on_config!=NULL)
    {
        on_config(&mqtt_cfg);
    }

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

}

//停止mqtt
void mqttc_stop()
{

    if(client!=NULL)
    {
        esp_mqtt_client_stop(client);
        esp_mqtt_client_destroy(client);
        client=NULL;
    }

}

//publish mqtt消息(包装库函数)
bool mqttc_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    if(client!=NULL)
    {
        return 0<=esp_mqtt_client_publish(client,topic,data,len,qos,retain);
    }

    return false;
}

#endif // CONFIG_NETWORK_PROTOCAL_MQTT
