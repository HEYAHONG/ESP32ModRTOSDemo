file(GLOB  SOURCE_C_CPP_FILES  *.c   *.cpp   mesh_netif/*.c mesh_netif/*.cpp  appstack/*.c appstack/*.cpp )
set(COMPONENT_SRCS ${SOURCE_C_CPP_FILES})
set(COMPONENT_ADD_INCLUDEDIRS "." "mesh_netif" "appstack" )

#默认框架中为启用tftp,在此处启用
set(COMPONENT_SRCS ${COMPONENT_SRCS} tftp/tftp_server.c)

register_component()

# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)


#添加SimpleMQTTGateWayStack库
add_subdirectory(SimpleMQTTGateWayStack/lib EXCLUDE_FROM_ALL)
target_link_libraries(${COMPONENT_LIB} SMGS)

#添加RC库
add_subdirectory(rc EXCLUDE_FROM_ALL)
target_link_libraries(${COMPONENT_LIB} RC)
