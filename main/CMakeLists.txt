set(COMPONENT_SRCS "main.c"  "tftpd.c" "init.c" "wifinetwork.c" "mqtt.c" "wifimeshnet.c" "mesh_netif/mesh_netif.c" "ethernet.c")
set(COMPONENT_ADD_INCLUDEDIRS "." "mesh_netif" )

#默认框架中为启用tftp,在此处启用
set(COMPONENT_SRCS ${COMPONENT_SRCS} tftp/tftp_server.c)

register_component()

# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
