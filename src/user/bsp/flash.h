#ifndef __FLASH_H__
#define __FLASH_H__
/*******************************************************************************************************/
#include "stdint.h"

#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE			(2048)              //FLASH页大小,必须字对齐
#endif

#define FLASH_PAGE_WORD			(512)               //FLASH页大小,字长度

#ifdef FLASH_SIZE_512K
//FLASH分区
#define FLASH_BOOT_BASE    		(0x08000000)        //bootloader起始地址,64K - 64B								
#define FLASH_APP_BASE    		(0x08007FC0)        //应用程序起始地址,64B + 208KB
#define FLASH_APP_ENTRY         (0x08008000)        //应用程序入口
#define FLASH_BAK_BASE    		(0x0803C000)        //数据后备区
#define FLASH_CFG_BASE          (0x0807F000)        //508K-512K,保存一些重要的配置信息,4K

#define FLASH_BOOT_SIZE			(32 * 1024 - 64)    //bootloader长度
#define FLASH_APP_SIZE			(208 * 1024 + 64)   //应用程序长度
#define FLASH_BAK_SIZE			(268 * 1024)        //数据后备区长度,差分升级时,再分为60K + 208K,其中60K为差分数据区,208K为合并后数据区
#define FLASH_CFG_SIZE          (4 * 1024)          //配置信息长度
#endif

#ifdef FLASH_SIZE_1024K
//FLASH分区
#define FLASH_BOOT_BASE    		(0x08000000)        //bootloader起始地址,32K - 64B								
#define FLASH_APP_BASE    		(0x08007FC0)        //应用程序起始地址,64B + 464KB
#define FLASH_APP_ENTRY         (0x08008000)        //应用程序入口
#define FLASH_BAK_BASE    		(0x0807C000)        //数据后备区
#define FLASH_CFG_BASE          (0x080FF000)        //1020K-1024K,保存一些重要的配置信息,4K

#define FLASH_BOOT_SIZE			(32 * 1024 - 64)    //bootloader长度
#define FLASH_APP_SIZE			(464 * 1024 + 64)   //应用程序长度
#define FLASH_BAK_SIZE			(524 * 1024)        //数据后备区长度,差分升级时,再分为60K + 464K,其中60K为差分数据区,464K为合并后数据区
#define FLASH_CFG_SIZE          (4 * 1024)          //配置信息长度
#endif



/*******************************************************************************************************/
typedef enum
{
    flash_boot,
    flash_app,
    flash_bak,
    flash_cfg
} flash_partition_enum;

/*******************************************************************************************************/
int flash_partition_addr(flash_partition_enum part, uint32_t *addr);
uint32_t flash_partition_size(flash_partition_enum part);
int flash_partition_erase(flash_partition_enum part, uint32_t addr, uint32_t size);
int flash_write_without_erase(flash_partition_enum part, uint32_t addr, const void *p, uint32_t len);
int flash_write(flash_partition_enum part, uint32_t addr, const void *p, uint32_t len);
int flash_read(flash_partition_enum part, uint32_t addr, void *p, uint32_t len);
uint8_t* flash_read_fp(flash_partition_enum part, uint32_t addr);
uint32_t flash_sum(uint32_t addr, uint32_t len);
uint32_t flash_crc(uint32_t addr, uint32_t len);
uint32_t flash_partition_crc(flash_partition_enum part, uint32_t addr, uint32_t len);

#endif
/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
