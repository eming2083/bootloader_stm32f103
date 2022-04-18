/**
  ******************************************************************************
  * @file    iap.c
  * @author  eming
  * @version V1.0.0
  * @date    2022-03-21
  * @brief   IAP����,֧��ѹ����ʽ�̼��Ͳ������
  ******************************************************************************
  */  
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "usr_cfg.h"
#include "stm32f10x.h"
#include "core_cm3.h"
#include "bsp.h"
#include "flash.h"
#include "sys_time.h"
#include "mylib.h"
#include "db.h"
#include "iap.h"
#include "shell.h"
#include "lzma_decompress.h"
#include "crc32.h"
#include "bspatch.h"
#include "vFile.h"

/*************************************************************/
#define IH_MAGIC   0x27051956   /* Image Magic Number       */
#define IH_NMLEN   (32 - 4)     /* Image Name Length        */

typedef struct image_header 
{
    uint32_t ih_magic;/* Image Header Magic Number     */
    uint32_t ih_hcrc; /* Image Header CRC Checksum     */
    uint32_t ih_time; /* Image Creation Timestamp */
    uint32_t ih_size; /* Image Data Size     */
    uint32_t ih_load; /* Data Load Address         */
    uint32_t ih_ep;   /* Entry Point Address      */
    uint32_t ih_dcrc; /* Image Data CRC Checksum */
    uint8_t  ih_os;   /* Operating System         */
    uint8_t  ih_arch; /* CPU architecture         */
    uint8_t  ih_type; /* Image Type          */
    uint8_t  ih_comp; /* Compression Type         */
    uint8_t  ih_name[IH_NMLEN]; /* Image Name      */
    uint32_t ih_ocrc; /* Old Image Data CRC Checksum */ 
} image_header_t;

#define UIMAGE_HEADER_LEN   sizeof(image_header_t)
#define UIMAGE_MAX_LEN      (0x80000) 

/**
 * @brief ����FLASH
 * @details �������ص�flash����
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void iap_download_flash_erase(void)
{
    flash_partition_erase(flash_bak, 0, FLASH_BAK_SIZE);
}

/**
 * @brief ��������
 * @details �������ݵ����ݷ���
 *
 * @param offset ����ƫ����
 * @param datp ����ָ��
 * @param datp ���ݳ���
 * @return �������
 *   @retval ��
 */
int iap_download(uint32_t offset, uint8_t *datp, uint16_t len)
{
    flash_write_without_erase(flash_bak, offset, datp, len);

    return(TRUE);
}

/**
 * @brief ��龵��
 * @details ����ͷ����ϢУ������,������ͷ
 *
 * @param part flash����
 * @return �������
 *   @retval ��
 */
static int iap_uimage_check(uint32_t addr, image_header_t *ih)
{
    uint32_t magic, size, dcrc, crc;
    int ret = FALSE;
    
    memcpy(ih, (void *)addr, UIMAGE_HEADER_LEN);
    magic = BigtoLittle32(ih->ih_magic);
    size  = BigtoLittle32(ih->ih_size);
    dcrc = BigtoLittle32(ih->ih_dcrc);
    
    if((magic == IH_MAGIC) && (size <= UIMAGE_MAX_LEN))
    {
        crc = flash_crc(addr + UIMAGE_HEADER_LEN, size);
        if(crc == dcrc)
        {
            ret = TRUE;
        }
    }
    
    return(ret);
}
    
/**
 * @brief �޸Ĺ̼�����
 * @details ���ǹ̼����ߴ��ֲ���
 *
 * @param ��
 * @return �������
 *   @retval ��
 */
int iap_image_patch(void)
{
    image_header_t ih;
    uint32_t uimage_addr;
    uint32_t size, dcrc, crc, ocrc;
    uint32_t app_addr, image_len, unpk_len;
    uint8_t *image_p, *patch_p;
    uint8_t *buff;
    vFile* vfp;
    uint32_t old_size;
    
    flash_partition_addr(flash_bak, &uimage_addr);
    if(TRUE == iap_uimage_check(uimage_addr, &ih))
    {
        size  = BigtoLittle32(ih.ih_size);
        dcrc = BigtoLittle32(ih.ih_dcrc);
        image_len = size + UIMAGE_HEADER_LEN;
        /*
        ** ��������:1,standalone; 2,kernel; 3,ramdisk; 4,multi; 5,firmware; 6,script; 7,filesystem; 8,diffpatch
        */
        if(ih.ih_type == 2)
        {
            /*
            ** ѹ������:0,none; 1,gzip; 2,bzip2; 3,lzma
            */
            if(ih.ih_comp == 0) //δѹ������,ֱ�Ӹ�д,mkuzimage -O rtos -T kernel -a 0x0800FFC0 -e 0x08010000 -i image.bin -o uimage.bin
            {
                flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);   //����APP
                //д��������
                image_p = flash_read_fp(flash_bak, IAP_BAK_ADDR);
                flash_write_without_erase(flash_app, 0, image_p, size + UIMAGE_HEADER_LEN);
                //��д���,��������CRC
                crc = flash_partition_crc(flash_app, 0 + UIMAGE_HEADER_LEN, size);
                if(crc == dcrc)
                {
                    return(TRUE);
                }
            }
            else if(ih.ih_comp == 3)    //LZMAѹ��,mkuzimage -O rtos -T kernel -C lzma -a 0x0800FFC0 -e 0x08010000 -i image.bin -o uzimage.bin
            {
                //�����ڴ�
                buff = (uint8_t *)malloc(FLASH_PAGE_SIZE);
                if(NULL == buff)return(FALSE);
                flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);   //����APP
                //��ѹuzimage�����ݶ�
                image_p = flash_read_fp(flash_bak, IAP_BAK_ADDR + UIMAGE_HEADER_LEN);
                vfp = vfopen(image_p, size);
                app_addr = UIMAGE_HEADER_LEN;
                image_len = 0;
                while(1)
                {
                    unpk_len = lzma_decompress_read(vfp, buff, FLASH_PAGE_SIZE);
                    if(unpk_len == 0)break;
                    flash_write_without_erase(flash_app, app_addr, buff, unpk_len);
                    app_addr += unpk_len;
                    image_len += unpk_len;
                }
                free(buff);
                //���д���ļ�ͷ
                crc = flash_partition_crc(flash_app, 0 + UIMAGE_HEADER_LEN, image_len);
                ih.ih_size = BigtoLittle32(image_len);
                ih.ih_dcrc = BigtoLittle32(crc);
                crc = crc32((unsigned char*)&ih, UIMAGE_HEADER_LEN);
                ih.ih_hcrc = BigtoLittle32(crc);
                ih.ih_comp = 0;
                flash_write_without_erase(flash_app, 0, &ih, UIMAGE_HEADER_LEN);
                
                return(TRUE);
            }
            else    //δ֧�ֵ�ѹ������
            {    
                return(FALSE);
            }
        }
        else if(ih.ih_type == 8)    //���������:mkuzimage -O rtthread -T diffpatch -C lzma -a 0 -e 0 -i diff.bin -o udiff.bin
        {
            if(ih.ih_comp == 3)     //LZMAѹ��
            {
                ocrc = BigtoLittle32(ih.ih_ocrc);
                //У���Ϲ̼���������
                flash_partition_addr(flash_app, &uimage_addr);
                if(TRUE == iap_uimage_check(uimage_addr, &ih))
                {
                    dcrc = BigtoLittle32(ih.ih_dcrc);
                    if(dcrc == ocrc)    //��鲹������ocrcֵ�Ƿ�����Ϲ̼���dcrc
                    {
                        old_size  = BigtoLittle32(ih.ih_size);
                        //����ְ���APP�ϲ�,���ݱ��浽������
                        flash_partition_erase(flash_bak, IAP_DIFF_NEW_ADDR, FLASH_BAK_SIZE - IAP_DIFF_NEW_ADDR);
                        image_p = flash_read_fp(flash_app, 0 + UIMAGE_HEADER_LEN);
                        patch_p = flash_read_fp(flash_bak, IAP_BAK_ADDR);
                        image_len = iap_patch(image_p, old_size, patch_p + UIMAGE_HEADER_LEN, size, IAP_DIFF_NEW_ADDR + UIMAGE_HEADER_LEN);
                        //�����ļ�ͷ��Ϣ
                        crc = flash_partition_crc(flash_bak, IAP_DIFF_NEW_ADDR + UIMAGE_HEADER_LEN, image_len);
                        ih.ih_size = BigtoLittle32(image_len);
                        ih.ih_dcrc = BigtoLittle32(crc);
                        crc = crc32((unsigned char*)&ih, UIMAGE_HEADER_LEN);
                        ih.ih_hcrc = BigtoLittle32(crc);
                        flash_write_without_erase(flash_bak, IAP_DIFF_NEW_ADDR, &ih, UIMAGE_HEADER_LEN);
                        
                        //�����Ϲ̼�
                        flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);
                        //д��������
                        image_p = flash_read_fp(flash_bak, IAP_DIFF_NEW_ADDR);
                        flash_write_without_erase(flash_app, 0, image_p, image_len + UIMAGE_HEADER_LEN);
                        
                        return(TRUE);                        
                    }                    
                }
                else
                {
                    //�����������,�������¹̼���дʱ,���ڶϵ���߸�λ�����Ϲ̼����ݱ��ƻ���,����д��
                    flash_partition_addr(flash_bak, &uimage_addr);
                    if(TRUE == iap_uimage_check(uimage_addr + IAP_DIFF_NEW_ADDR, &ih))
                    {
                        image_len  = BigtoLittle32(ih.ih_size);
                        //�����Ϲ̼�
                        flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);
                        //д��������
                        image_p = flash_read_fp(flash_bak, IAP_DIFF_NEW_ADDR);
                        flash_write_without_erase(flash_app, 0, image_p, image_len + UIMAGE_HEADER_LEN);
                        
                        return(TRUE); 
                    }
                }
            }
        }
    }

    return(FALSE);
}

/**
 * @brief �����̼�
 * @details ���������־,��Ч���������
 *
 * @param ��
 * @return �������
 *   @retval ��
 */
int iap_upgrade(void)
{
    IAP_AppBak_stk file_bak;

    db_read(&file_bak, DB_APPBAK);        //��ȡ����������Ϣ

    if(IAP_MAGIC_FLG == file_bak.flg)     //���������־
    {
        printk("upgrade beginning!\r\n");
        if(TRUE == iap_image_patch())
        {
            //������������Ϣ
            file_bak.flg = 0xFFFF;
            db_write(&file_bak, DB_APPBAK);
            printk("\r\nupgrade done!\r\n");
            
            return(TRUE);            
        }
        else
        {
            printk("upgrade failure!\r\n");
        }
    }
    return(FALSE);
}

/**
 * @brief ���ص�APP
 * @details ��ת��ָ����ַ���г���
 *
 * @param entry ��ڵ�ַ
 * @return �������
 *   @retval ��
 */
void iap_boot_app(uint32_t entry)
{
    typedef void (*pFunction)(void);
    pFunction Jump_To_Application;
    uint32_t 	JumpAddress;
    
    if (((*(volatile uint32_t *)entry) & 0x2FFE0000 ) == 0x20000000)
    {
        //�ͷ�ʹ�õ�Ӳ������
        board_deinit();
        __disable_irq();
        
        //��ת���û�����
        JumpAddress = *(volatile uint32_t *) (entry + 4);
        Jump_To_Application = (pFunction) JumpAddress;

        //��ʼ���û�����Ķ�ջָ��
        __set_MSP(*(volatile uint32_t *) entry);
        Jump_To_Application();
        
        while(1);
    }
}

/**
 * @brief ���ؾ���
 * @details ��龵��ͷ,CRCУ��,�Ϸ�����ؾ���
 *
 * @param ��
 * @return �������
 *   @retval ��
 */
static int iap_load_uimage(void)
{
    image_header_t ih;
    uint32_t uimage_addr;

    flash_partition_addr(flash_app, &uimage_addr); 
    if(TRUE == iap_uimage_check(uimage_addr, &ih))
    {
        printk("find image %s...\r\n", ih.ih_name);
        iap_boot_app(FLASH_APP_BASE + UIMAGE_HEADER_LEN);
    }
    else
    {
        printk("image file crc check failure!\r\n");
    }

    return(FALSE);
}

/**
 * @brief IAP������
 * @details 3S�ڿɱ��ж�����
 *
 * @param ��
 * @return �������
 *   @retval ��
 */
static int iap_task_ps = 0;
void iap_task(void)
{
    static int iap_task_delay;
    
    switch(iap_task_ps)
    {
    case 0:
        iap_task_delay = 1;
        printk("Hit any key to stop autoboot: %d", iap_task_delay);
        iap_task_ps = 1;
        break;
    
    case 1:
        iap_task_delay--;
        printk("\b");
        printk("%d", iap_task_delay);    
        if(iap_task_delay <= 0)
        {
            printk("\r\n");
            iap_task_ps = 2;
        }
        break;
    
    case 2:
        iap_upgrade();
        //���ؽ���Ӧ�ó���
        printk("boot from flash...\r\n");
        if(FALSE == iap_load_uimage())
        {
            printk("boot failure!\r\n");
            iap_task_ps = 0xFF;
        }        
        break;
        
    case 0xFF:
    default:
        break;
    }
}

/**
 * @brief �˳�IAP
 * @details ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void iap_quit(void)
{
    iap_task_ps = 0xFF;
}

/**
 * @brief �������ݽ���
 * @details У�����ݣ���д��������־
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void iap_download_done(void)
{
    image_header_t ih;
    uint32_t magic;
    IAP_AppBak_stk 	file_bak;
    
    flash_read(flash_bak, IAP_BAK_ADDR, (uint8_t*)&ih, UIMAGE_HEADER_LEN);
    magic = BigtoLittle32(ih.ih_magic);
    
    if(magic == IH_MAGIC)
    {
        file_bak.flg = IAP_MAGIC_FLG;     //������־
        db_write(&file_bak, DB_APPBAK);
    } 
}

/**
 * @brief IAP��������
 * @details �ֶ�����IAPģʽ
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void iap_upgrade_start(void)
{
    iap_download_done();   
}

#if CONFIG_SHELL_ENABLE
/**
 * @brief ֱ����ת��ָ����ַ
 * @details shell����,���ڵ���
 *
 * @param arg ���������ַ���
 * @return �������
 *   @retval ��
 */
int bootm(void *arg)
{
    static const char tips[] = "bootm [entry, eg 0x8008000]\r\n";
    char *argv[2];
    uint32_t entry;
    int argc = cmdline_strtok((char *)arg, argv, 2);
    
    if(argc < 2)
    {
        printl((char *)tips, sizeof(tips) - 1);
        entry = 0x8008000;
    }
    else
    {
        entry = htoi(argv[1]);
    }
    iap_boot_app(entry);
    
    return 0;
}
#endif

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/

