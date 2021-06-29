set(COMPONENT_SRCS "main.c"  "tftpd.c" "init.c")
set(COMPONENT_ADD_INCLUDEDIRS "." )

#默认框架中为启用tftp,在此处启用
set(COMPONENT_SRCS ${COMPONENT_SRCS} tftp/tftp_server.c)

register_component()
