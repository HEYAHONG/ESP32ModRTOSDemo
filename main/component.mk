#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_ADD_INCLUDEDIRS := include mesh_netif

COMPONENT_OBJS = main.o  tftpd.o init.o wifinetwork.o wifimeshnet.o mesh_netif/mesh_netif.o mqtt.o


#默认框架中为启用tftp,在此处启用
COMPONENT_OBJS += tftp/tftp_server.o

COMPONENT_SRCDIRS :=  ./  mesh_netif

#默认框架中为启用tftp,在此处启用
COMPONENT_SRCDIRS += tftp

COMPONENT_SUBMODULES += main

