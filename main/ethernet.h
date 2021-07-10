#ifndef ETHERNET_H_INCLUDED
#define ETHERNET_H_INCLUDED

#include "appconfig.h"
#include "esp_netif.h"
#include "esp_eth.h"
#if CONFIG_ETH_USE_SPI_ETHERNET
#include "driver/spi_master.h"
#endif // CONFIG_ETH_USE_SPI_ETHERNET

#if CONFIG_ETHERNET_NETWORK == 1

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef struct
{
    void (*eth_event_handler)(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
    void (*got_ip_event_handler)(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
} ethernet_network_callback_t;

//初始化ethernet
void ethernet_network_init(ethernet_network_callback_t cb);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CONFIG_ETHERNET_NETWORK

#endif // ETHERNET_H_INCLUDED
