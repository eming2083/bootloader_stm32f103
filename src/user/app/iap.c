/**
  ******************************************************************************
  * @file    iap.c
  * @author  eming
  * @version V1.0.0
  * @date    2022-03-21
  * @brief   IAP程序,支持压缩格式固件和差分升级
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
 * @brief 擦除FLASH
 * @details 擦除下载的flash分区
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
void iap_download_flash_erase(void)
{
    flash_partition_erase(flash_bak, 0, FLASH_BAK_SIZE);
}

/**
 * @brief 下载数据
 * @details 下载数据到备份分区
 *
 * @param offset 数据偏移量
 * @param datp 数据指针
 * @param datp 数据长度
 * @return 操作结果
 *   @retval 略
 */
int iap_download(uint32_t offset, uint8_t *datp, uint16_t len)
{
    flash_write_without_erase(flash_bak, offset, datp, len);

    return(TRUE);
}

/**
 * @brief 检查镜像
 * @details 根据头部信息校验数据,并返回头
 *
 * @param part flash分区
 * @return 操作结果
 *   @retval 略
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
 * @brief 修改固件镜像
 * @details 覆盖固件或者打差分补丁
 *
 * @param 无
 * @return 操作结果
 *   @retval 略
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
        ** 镜像类型:1,standalone; 2,kernel; 3,ramdisk; 4,multi; 5,firmware; 6,script; 7,filesystem; 8,diffpatch
        */
        if(ih.ih_type == 2)
        {
            /*
            ** 压缩类型:0,none; 1,gzip; 2,bzip2; 3,lzma
            */
            if(ih.ih_comp == 0) //未压缩镜像,直接覆写,mkuzimage -O rtos -T kernel -a 0x0800FFC0 -e 0x08010000 -i image.bin -o uimage.bin
            {
                flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);   //擦除APP
                //写镜像数据
                image_p = flash_read_fp(flash_bak, IAP_BAK_ADDR);
                flash_write_without_erase(flash_app, 0, image_p, size + UIMAGE_HEADER_LEN);
                //覆写完成,计算数据CRC
                crc = flash_partition_crc(flash_app, 0 + UIMAGE_HEADER_LEN, size);
                if(crc == dcrc)
                {
                    return(TRUE);
                }
            }
            else if(ih.ih_comp == 3)    //LZMA压缩,mkuzimage -O rtos -T kernel -C lzma -a 0x0800FFC0 -e 0x08010000 -i image.bin -o uzimage.bin
            {
                //分配内存
                buff = (uint8_t *)malloc(FLASH_PAGE_SIZE);
                if(NULL == buff)return(FALSE);
                flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);   //擦除APP
                //解压uzimage的数据段
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
                //最后写入文件头
                crc = flash_partition_crc(flash_app, 0 + UIMAGE_HEADER_LEN, image_len);
                ih.ih_size = BigtoLittle32(image_len);
                ih.ih_dcrc = BigtoLittle32(crc);
                crc = crc32((unsigned char*)&ih, UIMAGE_HEADER_LEN);
                ih.ih_hcrc = BigtoLittle32(crc);
                ih.ih_comp = 0;
                flash_write_without_erase(flash_app, 0, &ih, UIMAGE_HEADER_LEN);
                
                return(TRUE);
            }
            else    //未支持的压缩类型
            {    
                return(FALSE);
            }
        }
        else if(ih.ih_type == 8)    //差分升级包:mkuzimage -O rtthread -T diffpatch -C lzma -a 0 -e 0 -i diff.bin -o udiff.bin
        {
            if(ih.ih_comp == 3)     //LZMA压缩
            {
                ocrc = BigtoLittle32(ih.ih_ocrc);
                //校验老固件的完整性
                flash_partition_addr(flash_app, &uimage_addr);
                if(TRUE == iap_uimage_check(uimage_addr, &ih))
                {
                    dcrc = BigtoLittle32(ih.ih_dcrc);
                    if(dcrc == ocrc)    //检查补丁包的ocrc值是否等于老固件的dcrc
                    {
                        old_size  = BigtoLittle32(ih.ih_size);
                        //将差分包和APP合并,数据保存到备份区
                        flash_partition_erase(flash_bak, IAP_DIFF_NEW_ADDR, FLASH_BAK_SIZE - IAP_DIFF_NEW_ADDR);
                        image_p = flash_read_fp(flash_app, 0 + UIMAGE_HEADER_LEN);
                        patch_p = flash_read_fp(flash_bak, IAP_BAK_ADDR);
                        image_len = iap_patch(image_p, old_size, patch_p + UIMAGE_HEADER_LEN, size, IAP_DIFF_NEW_ADDR + UIMAGE_HEADER_LEN);
                        //更新文件头信息
                        crc = flash_partition_crc(flash_bak, IAP_DIFF_NEW_ADDR + UIMAGE_HEADER_LEN, image_len);
                        ih.ih_size = BigtoLittle32(image_len);
                        ih.ih_dcrc = BigtoLittle32(crc);
                        crc = crc32((unsigned char*)&ih, UIMAGE_HEADER_LEN);
                        ih.ih_hcrc = BigtoLittle32(crc);
                        flash_write_without_erase(flash_bak, IAP_DIFF_NEW_ADDR, &ih, UIMAGE_HEADER_LEN);
                        
                        //擦除老固件
                        flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);
                        //写镜像数据
                        image_p = flash_read_fp(flash_bak, IAP_DIFF_NEW_ADDR);
                        flash_write_without_erase(flash_app, 0, image_p, image_len + UIMAGE_HEADER_LEN);
                        
                        return(TRUE);                        
                    }                    
                }
                else
                {
                    //出现这种情况,可能是新固件覆写时,由于断电或者复位导致老固件数据被破坏了,重新写入
                    flash_partition_addr(flash_bak, &uimage_addr);
                    if(TRUE == iap_uimage_check(uimage_addr + IAP_DIFF_NEW_ADDR, &ih))
                    {
                        image_len  = BigtoLittle32(ih.ih_size);
                        //擦除老固件
                        flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);
                        //写镜像数据
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
 * @brief 升级固件
 * @details 检查升级标志,有效则进入升级
 *
 * @param 无
 * @return 操作结果
 *   @retval 略
 */
int iap_upgrade(void)
{
    IAP_AppBak_stk file_bak;

    db_read(&file_bak, DB_APPBAK);        //读取升级数据信息

    if(IAP_MAGIC_FLG == file_bak.flg)     //检查升级标志
    {
        printk("upgrade beginning!\r\n");
        if(TRUE == iap_image_patch())
        {
            //更新下载区信息
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
 * @brief 加载到APP
 * @details 跳转到指定地址运行程序
 *
 * @param entry 入口地址
 * @return 操作结果
 *   @retval 无
 */
void iap_boot_app(uint32_t entry)
{
    typedef void (*pFunction)(void);
    pFunction Jump_To_Application;
    uint32_t 	JumpAddress;
    
    if (((*(volatile uint32_t *)entry) & 0x2FFE0000 ) == 0x20000000)
    {
        //释放使用的硬件外设
        board_deinit();
        __disable_irq();
        
        //跳转至用户代码
        JumpAddress = *(volatile uint32_t *) (entry + 4);
        Jump_To_Application = (pFunction) JumpAddress;

        //初始化用户程序的堆栈指针
        __set_MSP(*(volatile uint32_t *) entry);
        Jump_To_Application();
        
        while(1);
    }
}

/**
 * @brief 加载镜像
 * @details 检查镜像头,CRC校验,合法则加载镜像
 *
 * @param 无
 * @return 操作结果
 *   @retval 略
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
 * @brief IAP主程序
 * @details 3S内可被中断流程
 *
 * @param 无
 * @return 操作结果
 *   @retval 无
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
        //加载进入应用程序
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
 * @brief 退出IAP
 * @details 无
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
void iap_quit(void)
{
    iap_task_ps = 0xFF;
}

/**
 * @brief 下载数据结束
 * @details 校验数据，并写入升级标志
 *
 * @param 无
 * @return 无
 *   @retval 无
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
        file_bak.flg = IAP_MAGIC_FLG;     //升级标志
        db_write(&file_bak, DB_APPBAK);
    } 
}

/**
 * @brief IAP测试用例
 * @details 手动进入IAP模式
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
void iap_upgrade_start(void)
{
    iap_download_done();   
}

#if CONFIG_SHELL_ENABLE
/**
 * @brief 直接跳转到指定地址
 * @details shell命令,用于调试
 *
 * @param arg 操作命令字符串
 * @return 操作结果
 *   @retval 略
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

