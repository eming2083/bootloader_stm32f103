#ifndef __IAP_H__
#define __IAP_H__
/*******************************************************************************************************/
#define IAP_BAK_ADDR        0               //APP备份镜像保存位置,共252K
#define IAP_DIFF_NEW_ADDR   (60 * 1024)     //差分升级时,合并后文件保存地址

#define IAP_MAGIC_FLG       (0xAA55)        //APP升级标志

/*******************************************************************************************************/
#include "stdint.h"

typedef struct
{
    uint16_t flg;            //升级标志
} IAP_AppBak_stk;

/*******************************************************************************************************/
void iap_download_flash_erase(void);
int  iap_download(uint32_t offset, uint8_t *datp, uint16_t len);
void iap_download_done(void);
void iap_upgrade_start(void);
void iap_task(void);
void iap_quit(void);

#endif
/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
