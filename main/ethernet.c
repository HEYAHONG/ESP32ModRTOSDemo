#include "ethernet.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#if CONFIG_ETH_USE_SPI_ETHERNET
#include "driver/spi_master.h"
#endif // CONFIG_ETH_USE_SPI_ETHERNET

#if CONFIG_ETHERNET_NETWORK == 1

static ethernet_network_callback_t callback = {NULL, NULL};

static const char *TAG = "ethernet_network";

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (callback.eth_event_handler != NULL)
    {
        callback.eth_event_handler(arg, event_base, event_id, event_data);
    }

    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{

    if (callback.got_ip_event_handler != NULL)
    {
        callback.got_ip_event_handler(arg, event_base, event_id, event_data);
    }

    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

static esp_eth_handle_t eth_handle = NULL;

void ethernet_network_init(ethernet_network_callback_t cb)
{
    callback = cb;

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    // Set default handlers to process TCP/IP stuffs
    //ESP_ERROR_CHECK(esp_eth_set_default_handlers(eth_netif));
    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = CONFIG_ETHERNET_NETWORK_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = CONFIG_ETHERNET_NETWORK_ETH_PHY_RST_GPIO;
#if CONFIG_ETHERNET_NETWORK_USE_INTERNAL_ETHERNET == 1
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    esp32_emac_config.smi_mdc_gpio_num = CONFIG_ETHERNET_NETWORK_ETH_MDC_GPIO;
    esp32_emac_config.smi_mdio_gpio_num = CONFIG_ETHERNET_NETWORK_ETH_MDIO_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
#if CONFIG_ETHERNET_NETWORK_ETH_PHY_IP101 == 1
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
#elif CONFIG_ETHERNET_NETWORK_ETH_PHY_RTL8201 == 1
    esp_eth_phy_t *phy = esp_eth_phy_new_rtl8201(&phy_config);
#elif CONFIG_ETHERNET_NETWORK_ETH_PHY_LAN8720 ==1
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
#elif CONFIG_ETHERNET_NETWORK_ETH_PHY_DP83848 == 1
    esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);
#elif CONFIG_ETHERNET_NETWORK_ETH_PHY_KSZ8041 == 1
    esp_eth_phy_t *phy = esp_eth_phy_new_ksz8041(&phy_config);
#elif CONFIG_ETHERNET_NETWORK_ETH_PHY_KSZ8081 == 1
    esp_eth_phy_t *phy = esp_eth_phy_new_ksz8081(&phy_config);
#endif
#elif CONFIG_ETH_USE_SPI_ETHERNET == 1
    gpio_install_isr_service(0);
    spi_device_handle_t spi_handle = NULL;
    spi_bus_config_t buscfg =
    {
        .miso_io_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_MISO_GPIO,
        .mosi_io_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_MOSI_GPIO,
        .sclk_io_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_ETHERNET_NETWORK_ETH_SPI_HOST, &buscfg, 1));

#if CONFIG_ETHERNET_NETWORK_USE_KSZ8851SNL == 1
    spi_device_interface_config_t devcfg =
    {
        .mode = 0,
        .clock_speed_hz = CONFIG_ETHERNET_NETWORK_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_CS_GPIO,
        .queue_size = 20
    };
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_ETHERNET_NETWORK_ETH_SPI_HOST, &devcfg, &spi_handle));
    /* KSZ8851SNL ethernet driver is based on spi driver */
    eth_ksz8851snl_config_t ksz8851snl_config = ETH_KSZ8851SNL_DEFAULT_CONFIG(spi_handle);
    ksz8851snl_config.int_gpio_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_INT_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_ksz8851snl(&ksz8851snl_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_ksz8851snl(&phy_config);
#elif CONFIG_ETHERNET_NETWORK_USE_DM9051 == 1
    spi_device_interface_config_t devcfg =
    {
        .command_bits = 1,
        .address_bits = 7,
        .mode = 0,
        .clock_speed_hz = CONFIG_ETHERNET_NETWORK_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_CS_GPIO,
        .queue_size = 20
    };
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_ETHERNET_NETWORK_ETH_SPI_HOST, &devcfg, &spi_handle));
    /* dm9051 ethernet driver is based on spi driver */
    eth_dm9051_config_t dm9051_config = ETH_DM9051_DEFAULT_CONFIG(spi_handle);
    dm9051_config.int_gpio_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_INT_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_dm9051(&dm9051_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_dm9051(&phy_config);
#elif CONFIG_ETHERNET_NETWORK_USE_W5500 == 1
    spi_device_interface_config_t devcfg =
    {
        .command_bits = 16, // Actually it's the address phase in W5500 SPI frame
        .address_bits = 8,  // Actually it's the control phase in W5500 SPI frame
        .mode = 0,
        .clock_speed_hz = CONFIG_ETHERNET_NETWORK_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_CS_GPIO,
        .queue_size = 20
    };
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_ETHERNET_NETWORK_ETH_SPI_HOST, &devcfg, &spi_handle));
    /* w5500 ethernet driver is based on spi driver */
    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle);
    w5500_config.int_gpio_num = CONFIG_ETHERNET_NETWORK_ETH_SPI_INT_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
#endif
#endif // CONFIG_ETH_USE_SPI_ETHERNET
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
#ifndef CONFIG_ETHERNET_NETWORK_USE_INTERNAL_ETHERNET
    /* The SPI Ethernet module might doesn't have a burned factory MAC address, we cat to set it manually.
       02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, (uint8_t[])
    {
        0x02, 0x00, 0x00, 0x12, 0x34, 0x56
    }));
#endif
    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    /* start Ethernet driver state machine */
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}


#endif // CONFIG_ETHERNET_NETWORK
