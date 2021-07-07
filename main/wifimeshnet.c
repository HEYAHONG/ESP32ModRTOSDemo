#include "wifimeshnet.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "nvs_flash.h"
#include "mesh_netif.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "init.h"
#include "string.h"

#if CONFIG_WIFI_MESH_NETWORK == 1




/*******************************************************
 *                Constants
 *******************************************************/
static const char *MESH_TAG = "mesh_network";
static const char MESH_ID[7] =CONFIG_MESH_ID;



/*******************************************************
 *                Variable Definitions
 *******************************************************/
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;
static esp_ip4_addr_t s_current_ip;

static wifimeshnet_callback_t wifimeshnet_callback= {NULL,NULL,NULL};

static void  recv_cb(mesh_addr_t *from, mesh_data_t *data)
{
    if(wifimeshnet_callback.receive_callback!=NULL)
    {
        wifimeshnet_callback.receive_callback(from,data);
    }

    return;
}

static void mesh_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if(wifimeshnet_callback.mesh_event_handler!=NULL)
    {
        wifimeshnet_callback.mesh_event_handler(arg,event_base,event_id,event_data);
    };

    mesh_addr_t id = {0,};
    static uint8_t last_layer = 0;

    switch (event_id)
    {
    case MESH_EVENT_STARTED:
    {
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
mesh_layer = esp_mesh_get_layer();
}
break;
    case MESH_EVENT_STOPPED:
{
ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
mesh_layer = esp_mesh_get_layer();
}
break;
    case MESH_EVENT_CHILD_CONNECTED:
{
mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
     child_connected->aid,
     MAC2STR(child_connected->mac));
}
break;
    case MESH_EVENT_CHILD_DISCONNECTED:
{
mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
     child_disconnected->aid,
     MAC2STR(child_disconnected->mac));
}
break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
{
mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
     routing_table->rt_size_change,
     routing_table->rt_size_new);
}
break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
{
mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
     routing_table->rt_size_change,
     routing_table->rt_size_new);
}
break;
    case MESH_EVENT_NO_PARENT_FOUND:
{
mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
     no_parent->scan_times);
}
    /* TODO handler for the failure */
break;
    case MESH_EVENT_PARENT_CONNECTED:
{
mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
esp_mesh_get_id(&id);
mesh_layer = connected->self_layer;
memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
ESP_LOGI(MESH_TAG,
     "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
     last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
     esp_mesh_is_root() ? "<ROOT>" :
     (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
last_layer = mesh_layer;
mesh_netifs_start(esp_mesh_is_root());
}
break;
    case MESH_EVENT_PARENT_DISCONNECTED:
{
mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
ESP_LOGI(MESH_TAG,
     "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
     disconnected->reason);
mesh_layer = esp_mesh_get_layer();
mesh_netifs_stop();
}
break;
    case MESH_EVENT_LAYER_CHANGE:
{
mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
mesh_layer = layer_change->new_layer;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
     last_layer, mesh_layer,
     esp_mesh_is_root() ? "<ROOT>" :
     (mesh_layer == 2) ? "<layer2>" : "");
last_layer = mesh_layer;
}
break;
    case MESH_EVENT_ROOT_ADDRESS:
{
mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
     MAC2STR(root_addr->addr));
}
break;
    case MESH_EVENT_VOTE_STARTED:
{
mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
ESP_LOGI(MESH_TAG,
     "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
     vote_started->attempts,
     vote_started->reason,
     MAC2STR(vote_started->rc_addr.addr));
}
break;
    case MESH_EVENT_VOTE_STOPPED:
{
ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
break;
}
    case MESH_EVENT_ROOT_SWITCH_REQ:
{
mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
ESP_LOGI(MESH_TAG,
     "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
     switch_req->reason,
     MAC2STR( switch_req->rc_addr.addr));
}
break;
    case MESH_EVENT_ROOT_SWITCH_ACK:
{
/* new root */
mesh_layer = esp_mesh_get_layer();
esp_mesh_get_parent_bssid(&mesh_parent_addr);
ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
}
break;
    case MESH_EVENT_TODS_STATE:
{
mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
}
break;
    case MESH_EVENT_ROOT_FIXED:
{
mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
     root_fixed->is_fixed ? "fixed" : "not fixed");
}
break;
    case MESH_EVENT_ROOT_ASKED_YIELD:
{
mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
ESP_LOGI(MESH_TAG,
     "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
     MAC2STR(root_conflict->addr),
     root_conflict->rssi,
     root_conflict->capacity);
}
break;
    case MESH_EVENT_CHANNEL_SWITCH:
{
mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
}
break;
    case MESH_EVENT_SCAN_DONE:
{
mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
     scan_done->number);
}
break;
    case MESH_EVENT_NETWORK_STATE:
{
mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
     network_state->is_rootless);
}
break;
    case MESH_EVENT_STOP_RECONNECTION:
{
ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
}
break;
    case MESH_EVENT_FIND_NETWORK:
{
mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
     find_network->channel, MAC2STR(find_network->router_bssid));
}
break;
    case MESH_EVENT_ROUTER_SWITCH:
{
mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
     router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
}
break;
    default:
        ESP_LOGI(MESH_TAG, "unknown id:%d", event_id);
        break;
    }
}

void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{

    if(wifimeshnet_callback.ip_event_handler!=NULL)
    {
        wifimeshnet_callback.ip_event_handler(arg,event_base,event_id,event_data);
    }

    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
    s_current_ip.addr = event->ip_info.ip.addr;
#if !CONFIG_MESH_USE_GLOBAL_DNS_IP
    esp_netif_t *netif = event->esp_netif;
    esp_netif_dns_info_t dns;
    ESP_ERROR_CHECK(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
    mesh_netif_start_root_ap(esp_mesh_is_root(), dns.ip.u_addr.ip4.addr);
#endif
}

//初始化wifimeshnet
void wifimeshnet_init(wifimeshnet_callback_t callback)
{

    wifimeshnet_callback=callback;

    /*  crete network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(mesh_netifs_init(recv_cb));

    /*  wifi initialization */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    /* router */
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
    memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,
           strlen(CONFIG_MESH_ROUTER_PASSWD));
    /* mesh softAP */
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
           strlen(CONFIG_MESH_AP_PASSWD));

    {//加载配置
        cJSON *obj=system_config_get_item("wifimeshnet");
        if(obj==NULL)
        {
            wifimeshnet_set_config(NULL);
        }
        else
        {
            if(cJSON_HasObjectItem(obj,"mesh_id"))
            {
                cJSON *item=cJSON_GetObjectItem(obj,"mesh_id");
                if(cJSON_IsArray(item))
                {
                    for(size_t i=0;i<6;i++)
                    {
                        if(i<cJSON_GetArraySize(item))
                        {
                            if(cJSON_IsNumber(cJSON_GetArrayItem(item,i)))
                            cfg.mesh_id.addr[i]=cJSON_GetNumberValue(cJSON_GetArrayItem(item,i));
                        }
                    }
                }
            }
            if(cJSON_HasObjectItem(obj,"mesh_ap_password"))
            {
                 cJSON *item=cJSON_GetObjectItem(obj,"mesh_ap_password");
                 if(cJSON_IsString(item))
                 {
                     char * str=cJSON_GetStringValue(item);
                     memcpy((uint8_t *) &cfg.mesh_ap.password, str,strlen(str));
                 }
            }
            if(cJSON_HasObjectItem(obj,"mesh_ap_max_connections"))
            {
                 cJSON *item=cJSON_GetObjectItem(obj,"mesh_ap_max_connections");
                 if(cJSON_IsNumber(item))
                 {
                     cfg.mesh_ap.max_connection=cJSON_GetNumberValue(item);
                 }
            }
            if(cJSON_HasObjectItem(obj,"mesh_channel"))
            {
                 cJSON *item=cJSON_GetObjectItem(obj,"mesh_channel");
                if(cJSON_IsNumber(item))
                 {
                     cfg.channel=cJSON_GetNumberValue(item);
                 }
            }
            if(cJSON_HasObjectItem(obj,"router_ssid"))
            {
                 cJSON *item=cJSON_GetObjectItem(obj,"router_ssid");
                 if(cJSON_IsString(item))
                 {
                     char * str=cJSON_GetStringValue(item);
                     memcpy((uint8_t *) &cfg.router.ssid, str,strlen(str));
                 }
            }
            if(cJSON_HasObjectItem(obj,"router_password"))
            {
                 cJSON *item=cJSON_GetObjectItem(obj,"router_password");
                 if(cJSON_IsString(item))
                 {
                     char * str=cJSON_GetStringValue(item);
                     memcpy((uint8_t *) &cfg.router.password, str,strlen(str));
                 }
            }
            if(cJSON_HasObjectItem(obj,"router_ssid_len"))
            {
                 cJSON *item=cJSON_GetObjectItem(obj,"router_ssid_len");
                 if(cJSON_IsNumber(item))
                 {
                     cfg.router.ssid_len=cJSON_GetNumberValue(item);
                 }
            }


            cJSON_Delete(obj);
        }

    }

    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
    ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s\n",  esp_get_free_heap_size(),
             esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed");


}


void wifimeshnet_set_config(wifimeshnet_config_t *cfg)
{
    wifimeshnet_config_t *config=cfg;
    if(cfg==NULL)
    {
        //默认配置
        config=(wifimeshnet_config_t *)malloc(sizeof(wifimeshnet_config_t));
        memset(config,0,sizeof(wifimeshnet_config_t));
        memcpy((uint8_t *) config->mesh_id, MESH_ID, 6);
        config->mesh_channel=CONFIG_MESH_CHANNEL;
        config->mesh_ap_max_connections=CONFIG_MESH_AP_CONNECTIONS;
        config->router_ssid_len=strlen(CONFIG_MESH_ROUTER_SSID);
        memcpy((uint8_t *) config->router_ssid, CONFIG_MESH_ROUTER_SSID,config->router_ssid_len);
        memcpy((uint8_t *) config->router_password, CONFIG_MESH_ROUTER_PASSWD,strlen(CONFIG_MESH_ROUTER_PASSWD));
        memcpy((uint8_t *) config->mesh_ap_password, CONFIG_MESH_AP_PASSWD,strlen(CONFIG_MESH_AP_PASSWD));

    }

    {//检查参数
        if(config->router_ssid_len==0)
        {
            config->router_ssid_len=strlen((char *)config->router_ssid);
        }

        if(config->mesh_ap_max_connections==0)
        {
            config->mesh_ap_max_connections=10;
        }
    }

    {
        cJSON *obj=system_config_get_item("wifimeshnet");
        if(obj==NULL)
        {
            obj=cJSON_CreateObject();
        }

        {//删除原有数据项
            if(cJSON_HasObjectItem(obj,"mesh_id"))
            {
                cJSON_DeleteItemFromObject(obj,"mesh_id");
            }
            if(cJSON_HasObjectItem(obj,"mesh_ap_password"))
            {
                cJSON_DeleteItemFromObject(obj,"mesh_ap_password");
            }
            if(cJSON_HasObjectItem(obj,"mesh_ap_max_connections"))
            {
                cJSON_DeleteItemFromObject(obj,"mesh_ap_max_connections");
            }
            if(cJSON_HasObjectItem(obj,"mesh_channel"))
            {
                cJSON_DeleteItemFromObject(obj,"mesh_channel");
            }
            if(cJSON_HasObjectItem(obj,"router_ssid"))
            {
                cJSON_DeleteItemFromObject(obj,"router_ssid");
            }
            if(cJSON_HasObjectItem(obj,"router_password"))
            {
                cJSON_DeleteItemFromObject(obj,"router_password");
            }
            if(cJSON_HasObjectItem(obj,"router_ssid_len"))
            {
                cJSON_DeleteItemFromObject(obj,"router_ssid_len");
            }
        }

        {//添加数据项

            {//mesh_id

                int mesh_id_int[6]={0};
                for(size_t i=0;i<6;i++)
                {
                    mesh_id_int[i]=config->mesh_id[i];
                }

                cJSON *mesh_id=cJSON_CreateIntArray(mesh_id_int,6);
                cJSON_AddItemToObject(obj,"mesh_id",mesh_id);
            }

            {//mesh_ap_password
                cJSON *mesh_ap_password=cJSON_CreateString((char *)config->mesh_ap_password);
                cJSON_AddItemToObject(obj,"mesh_ap_password",mesh_ap_password);
            }
            {//mesh_ap_max_connections
                cJSON *mesh_ap_max_connections=cJSON_CreateNumber(config->mesh_ap_max_connections);
                cJSON_AddItemToObject(obj,"mesh_ap_max_connections",mesh_ap_max_connections);
            }
            {//mesh_channel
                cJSON *mesh_channel=cJSON_CreateNumber(config->mesh_channel);
                cJSON_AddItemToObject(obj,"mesh_channel",mesh_channel);
            }

            {//router_ssid
                cJSON *router_ssid=cJSON_CreateString((char *)config->router_ssid);
                cJSON_AddItemToObject(obj,"router_ssid",router_ssid);
            }

            {//router_password
                cJSON *router_password=cJSON_CreateString((char *)config->router_password);
                cJSON_AddItemToObject(obj,"router_password",router_password);
            }

            {//router_ssid_len
                cJSON *router_ssid_len=cJSON_CreateNumber(config->router_ssid_len);
                cJSON_AddItemToObject(obj,"router_ssid_len",router_ssid_len);
            }


        }

        //保存数据
        system_config_put_item(obj,"wifimeshnet");
        system_config_save();

        cJSON_Delete(obj);
    }

    if(cfg==NULL)
    {
        free(config);
    }
}

#endif // CONFIG_WIFI_MESH_NETWORK
