#ifndef __FLASH_H__
#define __FLASH_H__
/*******************************************************************************************************/
#include "stdint.h"

#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE			(2048)              //FLASHҳ��С,�����ֶ���
#endif

#define FLASH_PAGE_WORD			(512)               //FLASHҳ��С,�ֳ���

#ifdef FLASH_SIZE_512K
//FLASH����
#define FLASH_BOOT_BASE    		(0x08000000)        //bootloader��ʼ��ַ,64K - 64B								
#define FLASH_APP_BASE    		(0x08007FC0)        //Ӧ�ó�����ʼ��ַ,64B + 208KB
#define FLASH_APP_ENTRY         (0x08008000)        //Ӧ�ó������
#define FLASH_BAK_BASE    		(0x0803C000)        //���ݺ���
#define FLASH_CFG_BASE          (0x0807F000)        //508K-512K,����һЩ��Ҫ��������Ϣ,4K

#define FLASH_BOOT_SIZE			(32 * 1024 - 64)    //bootloader����
#define FLASH_APP_SIZE			(208 * 1024 + 64)   //Ӧ�ó��򳤶�
#define FLASH_BAK_SIZE			(268 * 1024)        //���ݺ�������,�������ʱ,�ٷ�Ϊ60K + 208K,����60KΪ���������,208KΪ�ϲ���������
#define FLASH_CFG_SIZE          (4 * 1024)          //������Ϣ����
#endif

#ifdef FLASH_SIZE_1024K
//FLASH����
#define FLASH_BOOT_BASE    		(0x08000000)        //bootloader��ʼ��ַ,32K - 64B								
#define FLASH_APP_BASE    		(0x08007FC0)        //Ӧ�ó�����ʼ��ַ,64B + 464KB
#define FLASH_APP_ENTRY         (0x08008000)        //Ӧ�ó������
#define FLASH_BAK_BASE    		(0x0807C000)        //���ݺ���
#define FLASH_CFG_BASE          (0x080FF000)        //1020K-1024K,����һЩ��Ҫ��������Ϣ,4K

#define FLASH_BOOT_SIZE			(32 * 1024 - 64)    //bootloader����
#define FLASH_APP_SIZE			(464 * 1024 + 64)   //Ӧ�ó��򳤶�
#define FLASH_BAK_SIZE			(524 * 1024)        //���ݺ�������,�������ʱ,�ٷ�Ϊ60K + 464K,����60KΪ���������,464KΪ�ϲ���������
#define FLASH_CFG_SIZE          (4 * 1024)          //������Ϣ����
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
