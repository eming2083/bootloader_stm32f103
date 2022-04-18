/**
  ******************************************************************************
  * @file    flash.c
  * @author  eming
  * @version V1.0.0
  * @date    2021-09-19
  * @brief   内部FLASH初始化.
  ******************************************************************************
  */

#include <string.h>
#include <stdlib.h>
#include "ustdio.h"
#include "stm32f10x.h"
#include "usr_cfg.h"
#include "type.h"
#include "flash.h"
#include "mylib.h"

/*************************************************************/
extern void wdog_reload(void);


/**
 * @brief 获取FLASH数据地址
 * @details 将FALSH分区地址转换为物理地址
 *
 * @param  part 分区号
 * @param  addr 分区内数据相对地址
 * @param  len 要操作的数据长度
 * @param  *phy_addr 返回的物理地址
 * @return 操作的数据是否在分区内
 *   @retval TRUE 合法的操作
 *   @retval FALSE 非法的操作
 */
static int flash_addr(flash_partition_enum part, uint32_t addr, uint32_t len, uint32_t *phy_addr)
{
    switch(part)                               //检查存储类型
    {
    case flash_boot:
        if((addr + len) > FLASH_BOOT_SIZE)return(FALSE);
        *phy_addr = FLASH_BOOT_BASE + addr;
        break;

    case flash_app:
        if((addr + len) > FLASH_APP_SIZE)return(FALSE);
        *phy_addr = FLASH_APP_BASE + addr;
        break;
    
    case flash_bak:
        if((addr + len) > FLASH_BAK_SIZE)return(FALSE);
        *phy_addr = FLASH_BAK_BASE + addr;
        break;    

    case flash_cfg:
        if((addr + len) > FLASH_CFG_SIZE)return(FALSE);
        *phy_addr = FLASH_CFG_BASE + addr;
        break;

    default:
        return(FALSE);
    }
    return(TRUE);
}

/**
 * @brief 获取分区起始地址
 * @details 获取指定分区的起始地址
 *
 * @param  part 分区号
 * @return 分区起始地址
 *   @retval 无
 */
int flash_partition_addr(flash_partition_enum part, uint32_t *addr)
{
    int ret = TRUE;
    
    switch(part)                               //检查存储类型
    {
    case flash_boot:
        *addr = FLASH_BOOT_BASE;
        break;

    case flash_app:
        *addr = FLASH_APP_BASE;
        break;
    
    case flash_bak:
        *addr = FLASH_BAK_BASE;
        break;    

    case flash_cfg:
        *addr = FLASH_CFG_BASE;
        break;
    
    default:
        ret = FALSE;
        break;
    }
    
    return(ret);
}

/**
 * @brief 获取分区大小
 * @details 获取指定分区的字节数
 *
 * @param  part 分区号
 * @return 分区大小
 *   @retval 无
 */
uint32_t flash_partition_size(flash_partition_enum part)
{
    uint32_t size = 0;
    
    switch(part)                               //检查存储类型
    {
    case flash_boot:
        size = FLASH_BOOT_SIZE;
        break;

    case flash_app:
        size = FLASH_APP_SIZE;
        break;
    
    case flash_bak:
        size = FLASH_BAK_SIZE;
        break;    

    case flash_cfg:
        size = FLASH_CFG_SIZE;;
        break;
    }
    
    return(size);
}

/**
 * @brief 按页写数据
 * @details 写入整页数据
 *
 * @param  page_addr 页地址,4字节对齐
 * @param  *datp 数据指针
 * @return 操作结果
 *   @retval 无
 */
static int flash_page_write(uint32_t page_addr, uint8_t *datp)
{
    volatile uint32_t *fls_p;
    uint32_t i, word;
    U32_U8 dat;

    page_addr &= 0xFFFFFFFC;
    fls_p 	 = (volatile uint32_t *)page_addr;
    for(i = 0; i < FLASH_PAGE_WORD; i++)	  //写入数据
    {
        dat.d8[0] = *(datp++);
        dat.d8[1] = *(datp++);
        dat.d8[2] = *(datp++);
        dat.d8[3] = *(datp++);
        word = *(fls_p++);
        if((dat.d32 != word) && (word == 0xFFFFFFFF))
        {
            FLASH_ProgramWord(page_addr, dat.d32); //按字写入
        }
        page_addr += 4;
    }
    return(TRUE);
}

/**
 * @brief 按页读数据
 * @details 读出整页数据
 *
 * @param  page_addr 页地址,4字节对齐
 * @param  *datp 数据指针
 * @return 页大小
 *   @retval 无
 */
static int flash_page_read(uint32_t page_addr, uint8_t *datp)
{
    volatile uint32_t *fls_p;
    uint32_t i, *dp32;

    page_addr &= 0xFFFFFFFC;
    fls_p  = (volatile uint32_t *)page_addr;
    dp32   = (uint32_t *)datp;
    for(i = 0; i < FLASH_PAGE_WORD; i++)			//读取数据
    {
        *(dp32++) = *(fls_p++);
    }
    
    return(FLASH_PAGE_SIZE);
}

/**
 * @brief 扇区擦除
 * @details 扇区擦除,支持非扇区对齐
 *
 * @param  addr,起始地址
 * @param  len,擦除的数据块长度
 * @return erase_len 本扇区擦除的长度,可能小于len
 *   @retval 无 
 */
static uint32_t flash_page_erase(uint32_t addr, uint32_t len)
{
    uint32_t page_addr, offset;
    uint8_t  *buff;
    uint32_t erase_len;
    
    offset = addr % FLASH_PAGE_SIZE;        //计算是否扇区对齐
    erase_len = FLASH_PAGE_SIZE - offset;   //本扇区可擦除的长度
    if(erase_len > len)erase_len = len;
    
    //如果擦除长度小于扇区长度,说明有数据需要保留
    if(erase_len < FLASH_PAGE_SIZE)
    {
        //申请内存
        buff = (uint8_t *)malloc(FLASH_PAGE_SIZE);
        if(NULL == buff)return(FALSE);
        page_addr = addr - offset;
        flash_page_read(page_addr, buff);	//读取数据
        FLASH_ErasePage(addr);
        memset(buff + offset, 0xFF, erase_len);
        flash_page_write(page_addr, buff); //写入数据
        free(buff);
    }
    else
    {
        FLASH_ErasePage(addr);
        erase_len = FLASH_PAGE_SIZE;
    }
    
    return(erase_len);
}

/**
 * @brief 分区擦除
 * @details 擦除整个分区,类似硬盘格式化
 *
 * @param  part 分区号
 * @param  addr,起始地址
 * @param  size,擦除的数据块长度
 * @return 操作结果
 *   @retval 无 
 */
int flash_partition_erase(flash_partition_enum part, uint32_t addr, uint32_t size)
{
    uint32_t part_addr;
    uint32_t erase_len;

    if(FALSE == flash_addr(part, addr, size, &part_addr))return(FALSE);
    
    FLASH_Unlock();
    
    while(size > 0)
    {
        erase_len = flash_page_erase(part_addr, size);
        part_addr += erase_len;
        size -= erase_len;
        wdog_reload();
    }
    
    FLASH_Lock();
    
    return(TRUE);
}

/**
 * @brief 分区数据写入
 * @details 当前分区数据写入,不支持非对齐写入
 *
 * @param  part 分区号
 * @param  addr 相对地址
 * @param  p 数据指针
 * @param  len 数据长度
 * @return 操作结果
 *   @retval 无 
 */
int flash_write_without_erase(flash_partition_enum part, uint32_t addr, const void *p, uint32_t len)
{
    uint32_t phy_addr;
    U32_U8 dat;
    uint8_t *datp = (uint8_t*)p;
    int i;    
    
    if(FALSE == flash_addr(part, addr, len, &phy_addr))return(FALSE);
    
    wdog_reload();
    FLASH_Unlock();
    if((phy_addr % 4) != 0)
    {
        return(FALSE);
    }
    
    while(len)
    {
        dat.d32 = 0xFFFFFFFF;
        for(i = 0; i < 4; i++)
        {
            if(len > 0)
            {
                dat.d8[i] = *(datp++);
                len--;
            }
        }
        FLASH_ProgramWord(phy_addr, dat.d32); //按字写入
        phy_addr += 4;
        //每写入1K数据,喂一次狗
        if(0 == (phy_addr & 0x3FF))
        {
            wdog_reload();
        }
    }
    FLASH_Lock();
    
    return(TRUE);
}
    
/**
 * @brief 分区数据写入
 * @details 当前分区数据写入,支持非对齐写入,写之前不需要擦除
 *
 * @param  part 分区号
 * @param  addr 相对地址
 * @param  p 数据指针
 * @param  len 数据长度
 * @return 操作结果
 *   @retval 无 
 */
int flash_write(flash_partition_enum part, uint32_t addr, const void *p, uint32_t len)
{
    uint32_t pg, pd, offset;
    uint32_t page_addr, phy_addr;
    uint8_t  *buff;
    uint8_t *datp = (uint8_t*)p;

    if(FALSE == flash_addr(part, addr, len, &phy_addr))return(FALSE);

    wdog_reload();
    //申请内存
    buff = (uint8_t *)malloc(FLASH_PAGE_SIZE);
    if(NULL == buff)
    {
        printk("FLASH:memory allocation failure...\r\n");
        return(FALSE);
    }

    FLASH_Unlock();							    //解锁FLASH编写擦除控制器
    pg = phy_addr / FLASH_PAGE_SIZE;            //计算扇区号
    offset = phy_addr % FLASH_PAGE_SIZE;        //计算扇区内偏移
    pd = FLASH_PAGE_SIZE - offset;          	//计算当前地址所在的扇区还可以写多少字节
    if(len <= pd)pd = len;                 		//判断数据是否在当前页内
    page_addr = pg * FLASH_PAGE_SIZE;	        //计算扇区物理地址
    flash_page_read(page_addr, buff);		    //读取数据

    //判断要写入的区域是否为0xFF
    if((FALSE == mylib_memcmp_b(buff + offset, 0xFF, pd)) || (offset % 4))
    {
        FLASH_ErasePage(page_addr);		    //擦除一个扇区
    }
    memcpy(buff + offset, datp, pd);		    //修改数据
    flash_page_write(page_addr, buff);		    //写入数据

    len -= pd;                         		    //判断剩下的数据长度
    datp += pd;                                 //移动数据指针
    while(len)                            		//如果没有写完,写到下一页
    {
        pg++;                           		//扇区号指向下一扇区
        page_addr = pg * FLASH_PAGE_SIZE;	    //计算物理地址
        if(len >= FLASH_PAGE_SIZE)            	//如果剩下的数据大于扇区长度
        {
            flash_page_read(page_addr, buff);  //读取该扇区
            //判断要写入的区域是否为0xFF
            if(FALSE == mylib_memcmp_b(buff, 0xFF, FLASH_PAGE_SIZE))
            {
                FLASH_ErasePage(page_addr);    //擦除一个扇区
            }
            flash_page_write(page_addr, datp); //写入数据
            len -= FLASH_PAGE_SIZE;
            datp += FLASH_PAGE_SIZE;
        }
        else
        {
            flash_page_read(page_addr, buff);  //读取该扇区
            //判断要写入的区域是否为0xFF
            if(FALSE == mylib_memcmp_b(buff, 0xFF, len))
            {
                FLASH_ErasePage(page_addr);    //擦除一个扇区
            }
            memcpy(buff, datp, len);            //修改数据
            flash_page_write(page_addr, buff); //写入数据
            len = 0;
        }
    }
    FLASH_Lock();
    free(buff);
    
    return(TRUE);
}

/**
 * @brief 分区数据读取
 * @details 当前分区数据读取,M3支持字节对齐访问
 *
 * @param  part 分区号
 * @param  addr 相对地址
 * @param  p 数据指针
 * @param  len 数据长度
 * @return 操作结果
 *   @retval 无 
 */
int flash_read(flash_partition_enum part, uint32_t addr, void *p, uint32_t len)
{
    uint32_t phy_addr;
    uint8_t *datp;
    volatile uint8_t *fls_p;

    if(FALSE == flash_addr(part, addr, len, &phy_addr))return(FALSE);
    fls_p = (volatile uint8_t *)phy_addr;
    datp = (uint8_t*)p;

    while(len)
    {
        *(datp++) = *(fls_p++);
        len--;
    }
    return(TRUE);
}

/**
 * @brief 直接获取分区数据操作指针
 * @details 当前分区数据访问,M3支持字节对齐
 *
 * @param  part 分区号
 * @param  addr 相对地址
 * @return 数据指针
 *   @retval 无 
 */
uint8_t* flash_read_fp(flash_partition_enum part, uint32_t addr)
{
    uint32_t phy_addr;
    uint8_t *fls_p;
    
    if(FALSE == flash_addr(part, addr, 0, &phy_addr))return(NULL);
    fls_p = (uint8_t *)phy_addr;
    
    return(fls_p);
}

/**
 * @brief 计算和校验
 * @details 计算当前分区数据块和校验
 *
 * @param  part 分区号
 * @param  addr 数据起始地址
 * @param  len 数据长度
 * @return 和校验结果
 *   @retval 无 
 */
uint32_t flash_sum(uint32_t addr, uint32_t len)
{
    uint32_t sum;
    volatile uint8_t *fls_p;

    fls_p = (volatile uint8_t *)addr;
    
    sum = 0;
    while(len--)
    {
        sum += *(fls_p++);
        if(0 == (len & 0x3FF))			//每校验1K数据,喂一次狗
        {
            wdog_reload();
        }
    }
    return(FALSE);
}

/**
 * @brief 计算CRC32校验
 * @details 计算当前地址数据块CRC32校验
 *
 * @param  addr 数据起始地址
 * @param  len 数据长度
 * @return CRC32校验结果
 *   @retval 无 
 */
extern const unsigned int crc32tab[];
uint32_t flash_crc(uint32_t addr, uint32_t len)
{
    volatile uint8_t *fls_p;
    uint32_t crc;
    
    fls_p = (volatile uint8_t *)addr;
    
    crc = 0xFFFFFFFF;
    while(len--)
    {
        //计算CRC
        crc = crc32tab[(crc ^ (*(fls_p++))) & 0xff] ^ (crc >> 8);
        if(0 == (len & 0x3FF))			//每校验1K数据,喂一次狗
        {
            wdog_reload();
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief 计算CRC32校验
 * @details 计算当前分区数据块CRC32校验
 *
 * @param  part 分区号
 * @param  addr 数据起始地址
 * @param  len 数据长度
 * @return CRC32校验结果
 *   @retval 无 
 */
uint32_t flash_partition_crc(flash_partition_enum part, uint32_t addr, uint32_t len)
{
    uint32_t phy_addr;
    
    if(FALSE == flash_addr(part, addr, len, &phy_addr))return(FALSE);
    
    return flash_crc(phy_addr, len);
}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/

