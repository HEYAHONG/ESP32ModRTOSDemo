#include "tftpd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "lwip/apps/tftp_server.h"

/**
  * Open file for read/write.
  * @param fname Filename
  * @param mode Mode string from TFTP RFC 1350 (netascii, octet, mail)
  * @param write Flag indicating read (0) or write (!= 0) access
  * @returns File handle supplied to other functions
  */
static void *tftpd_open(const char *fname, const char *mode, u8_t write)
{
    const char *filemode = "r";
    if (write)
    {
        filemode = "w";
    }
    if (strcmp(mode, "octet") == 0)
    {
        filemode = "rb";
        if (write)
        {
            filemode = "wb";
        }
    }

    char *filename = (char *)malloc(strlen(fname) + 20);

    memset(filename, 0, strlen(fname) + 20);

    memcpy(filename, "/spiffs/", sizeof("/spiffs/")); //将读写转换至spiffs
    strcat(filename, fname);

    void *ret = fopen(filename, filemode);

    free(filename);

    return ret;
}
/**
 * Close file handle
 * @param handle File handle returned by open()
 */
static void tftpd_close(void *handle)
{
    if (handle == NULL)
    {
        return;
    }

    fclose((FILE *)handle);
}
/**
 * Read from file
 * @param handle File handle returned by open()
 * @param buf Target buffer to copy read data to
 * @param bytes Number of bytes to copy to buf
 * @returns &gt;= 0: Success; &lt; 0: Error
 */
static int tftpd_read(void *handle, void *buf, int bytes)
{
    if (handle == NULL)
    {
        return -1;
    }



    return fread(buf, 1, bytes, (FILE *)handle);
}
/**
 * Write to file
 * @param handle File handle returned by open()
 * @param pbuf PBUF adjusted such that payload pointer points
 *             to the beginning of write data. In other words,
 *             TFTP headers are stripped off.
 * @returns &gt;= 0: Success; &lt; 0: Error
 */
static int tftpd_write(void *handle, struct pbuf *p)
{
    if (handle == NULL)
    {
        return -1;
    }

    return fwrite(p->payload, 1, p->len, (FILE *)handle);
}

static struct tftp_context ctx =
{
    tftpd_open,
    tftpd_close,
    tftpd_read,
    tftpd_write
};

err_t tftpd_start()
{
    return tftp_init(&ctx);
}

err_t tftpd_stop()
{
    tftp_cleanup();
    return 0;
}
