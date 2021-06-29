#ifndef TFTPD_H_INCLUDED
#define TFTPD_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "lwip/apps/tftp_server.h"

err_t tftpd_start();

err_t tftpd_stop();


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // TFTPD_H_INCLUDED
