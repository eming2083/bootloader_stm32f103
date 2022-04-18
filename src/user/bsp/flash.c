/**
  ******************************************************************************
  * @file    flash.c
  * @author  eming
  * @version V1.0.0
  * @date    2021-09-19
  * @brief   �ڲ�FLASH��ʼ��.
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
 * @brief ��ȡFLASH���ݵ�ַ
 * @details ��FALSH������ַת��Ϊ�����ַ
 *
 * @param  part ������
 * @param  addr ������������Ե�ַ
 * @param  len Ҫ���������ݳ���
 * @param  *phy_addr ���ص������ַ
 * @return �����������Ƿ��ڷ�����
 *   @retval TRUE �Ϸ��Ĳ���
 *   @retval FALSE �Ƿ��Ĳ���
 */
static int flash_addr(flash_partition_enum part, uint32_t addr, uint32_t len, uint32_t *phy_addr)
{
    switch(part)                               //���洢����
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
 * @brief ��ȡ������ʼ��ַ
 * @details ��ȡָ����������ʼ��ַ
 *
 * @param  part ������
 * @return ������ʼ��ַ
 *   @retval ��
 */
int flash_partition_addr(flash_partition_enum part, uint32_t *addr)
{
    int ret = TRUE;
    
    switch(part)                               //���洢����
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
 * @brief ��ȡ������С
 * @details ��ȡָ���������ֽ���
 *
 * @param  part ������
 * @return ������С
 *   @retval ��
 */
uint32_t flash_partition_size(flash_partition_enum part)
{
    uint32_t size = 0;
    
    switch(part)                               //���洢����
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
 * @brief ��ҳд����
 * @details д����ҳ����
 *
 * @param  page_addr ҳ��ַ,4�ֽڶ���
 * @param  *datp ����ָ��
 * @return �������
 *   @retval ��
 */
static int flash_page_write(uint32_t page_addr, uint8_t *datp)
{
    volatile uint32_t *fls_p;
    uint32_t i, word;
    U32_U8 dat;

    page_addr &= 0xFFFFFFFC;
    fls_p 	 = (volatile uint32_t *)page_addr;
    for(i = 0; i < FLASH_PAGE_WORD; i++)	  //д������
    {
        dat.d8[0] = *(datp++);
        dat.d8[1] = *(datp++);
        dat.d8[2] = *(datp++);
        dat.d8[3] = *(datp++);
        word = *(fls_p++);
        if((dat.d32 != word) && (word == 0xFFFFFFFF))
        {
            FLASH_ProgramWord(page_addr, dat.d32); //����д��
        }
        page_addr += 4;
    }
    return(TRUE);
}

/**
 * @brief ��ҳ������
 * @details ������ҳ����
 *
 * @param  page_addr ҳ��ַ,4�ֽڶ���
 * @param  *datp ����ָ��
 * @return ҳ��С
 *   @retval ��
 */
static int flash_page_read(uint32_t page_addr, uint8_t *datp)
{
    volatile uint32_t *fls_p;
    uint32_t i, *dp32;

    page_addr &= 0xFFFFFFFC;
    fls_p  = (volatile uint32_t *)page_addr;
    dp32   = (uint32_t *)datp;
    for(i = 0; i < FLASH_PAGE_WORD; i++)			//��ȡ����
    {
        *(dp32++) = *(fls_p++);
    }
    
    return(FLASH_PAGE_SIZE);
}

/**
 * @brief ��������
 * @details ��������,֧�ַ���������
 *
 * @param  addr,��ʼ��ַ
 * @param  len,���������ݿ鳤��
 * @return erase_len �����������ĳ���,����С��len
 *   @retval �� 
 */
static uint32_t flash_page_erase(uint32_t addr, uint32_t len)
{
    uint32_t page_addr, offset;
    uint8_t  *buff;
    uint32_t erase_len;
    
    offset = addr % FLASH_PAGE_SIZE;        //�����Ƿ���������
    erase_len = FLASH_PAGE_SIZE - offset;   //�������ɲ����ĳ���
    if(erase_len > len)erase_len = len;
    
    //�����������С����������,˵����������Ҫ����
    if(erase_len < FLASH_PAGE_SIZE)
    {
        //�����ڴ�
        buff = (uint8_t *)malloc(FLASH_PAGE_SIZE);
        if(NULL == buff)return(FALSE);
        page_addr = addr - offset;
        flash_page_read(page_addr, buff);	//��ȡ����
        FLASH_ErasePage(addr);
        memset(buff + offset, 0xFF, erase_len);
        flash_page_write(page_addr, buff); //д������
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
 * @brief ��������
 * @details ������������,����Ӳ�̸�ʽ��
 *
 * @param  part ������
 * @param  addr,��ʼ��ַ
 * @param  size,���������ݿ鳤��
 * @return �������
 *   @retval �� 
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
 * @brief ��������д��
 * @details ��ǰ��������д��,��֧�ַǶ���д��
 *
 * @param  part ������
 * @param  addr ��Ե�ַ
 * @param  p ����ָ��
 * @param  len ���ݳ���
 * @return �������
 *   @retval �� 
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
        FLASH_ProgramWord(phy_addr, dat.d32); //����д��
        phy_addr += 4;
        //ÿд��1K����,ιһ�ι�
        if(0 == (phy_addr & 0x3FF))
        {
            wdog_reload();
        }
    }
    FLASH_Lock();
    
    return(TRUE);
}
    
/**
 * @brief ��������д��
 * @details ��ǰ��������д��,֧�ַǶ���д��,д֮ǰ����Ҫ����
 *
 * @param  part ������
 * @param  addr ��Ե�ַ
 * @param  p ����ָ��
 * @param  len ���ݳ���
 * @return �������
 *   @retval �� 
 */
int flash_write(flash_partition_enum part, uint32_t addr, const void *p, uint32_t len)
{
    uint32_t pg, pd, offset;
    uint32_t page_addr, phy_addr;
    uint8_t  *buff;
    uint8_t *datp = (uint8_t*)p;

    if(FALSE == flash_addr(part, addr, len, &phy_addr))return(FALSE);

    wdog_reload();
    //�����ڴ�
    buff = (uint8_t *)malloc(FLASH_PAGE_SIZE);
    if(NULL == buff)
    {
        printk("FLASH:memory allocation failure...\r\n");
        return(FALSE);
    }

    FLASH_Unlock();							    //����FLASH��д����������
    pg = phy_addr / FLASH_PAGE_SIZE;            //����������
    offset = phy_addr % FLASH_PAGE_SIZE;        //����������ƫ��
    pd = FLASH_PAGE_SIZE - offset;          	//���㵱ǰ��ַ���ڵ�����������д�����ֽ�
    if(len <= pd)pd = len;                 		//�ж������Ƿ��ڵ�ǰҳ��
    page_addr = pg * FLASH_PAGE_SIZE;	        //�������������ַ
    flash_page_read(page_addr, buff);		    //��ȡ����

    //�ж�Ҫд��������Ƿ�Ϊ0xFF
    if((FALSE == mylib_memcmp_b(buff + offset, 0xFF, pd)) || (offset % 4))
    {
        FLASH_ErasePage(page_addr);		    //����һ������
    }
    memcpy(buff + offset, datp, pd);		    //�޸�����
    flash_page_write(page_addr, buff);		    //д������

    len -= pd;                         		    //�ж�ʣ�µ����ݳ���
    datp += pd;                                 //�ƶ�����ָ��
    while(len)                            		//���û��д��,д����һҳ
    {
        pg++;                           		//������ָ����һ����
        page_addr = pg * FLASH_PAGE_SIZE;	    //���������ַ
        if(len >= FLASH_PAGE_SIZE)            	//���ʣ�µ����ݴ�����������
        {
            flash_page_read(page_addr, buff);  //��ȡ������
            //�ж�Ҫд��������Ƿ�Ϊ0xFF
            if(FALSE == mylib_memcmp_b(buff, 0xFF, FLASH_PAGE_SIZE))
            {
                FLASH_ErasePage(page_addr);    //����һ������
            }
            flash_page_write(page_addr, datp); //д������
            len -= FLASH_PAGE_SIZE;
            datp += FLASH_PAGE_SIZE;
        }
        else
        {
            flash_page_read(page_addr, buff);  //��ȡ������
            //�ж�Ҫд��������Ƿ�Ϊ0xFF
            if(FALSE == mylib_memcmp_b(buff, 0xFF, len))
            {
                FLASH_ErasePage(page_addr);    //����һ������
            }
            memcpy(buff, datp, len);            //�޸�����
            flash_page_write(page_addr, buff); //д������
            len = 0;
        }
    }
    FLASH_Lock();
    free(buff);
    
    return(TRUE);
}

/**
 * @brief �������ݶ�ȡ
 * @details ��ǰ�������ݶ�ȡ,M3֧���ֽڶ������
 *
 * @param  part ������
 * @param  addr ��Ե�ַ
 * @param  p ����ָ��
 * @param  len ���ݳ���
 * @return �������
 *   @retval �� 
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
 * @brief ֱ�ӻ�ȡ�������ݲ���ָ��
 * @details ��ǰ�������ݷ���,M3֧���ֽڶ���
 *
 * @param  part ������
 * @param  addr ��Ե�ַ
 * @return ����ָ��
 *   @retval �� 
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
 * @brief �����У��
 * @details ���㵱ǰ�������ݿ��У��
 *
 * @param  part ������
 * @param  addr ������ʼ��ַ
 * @param  len ���ݳ���
 * @return ��У����
 *   @retval �� 
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
        if(0 == (len & 0x3FF))			//ÿУ��1K����,ιһ�ι�
        {
            wdog_reload();
        }
    }
    return(FALSE);
}

/**
 * @brief ����CRC32У��
 * @details ���㵱ǰ��ַ���ݿ�CRC32У��
 *
 * @param  addr ������ʼ��ַ
 * @param  len ���ݳ���
 * @return CRC32У����
 *   @retval �� 
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
        //����CRC
        crc = crc32tab[(crc ^ (*(fls_p++))) & 0xff] ^ (crc >> 8);
        if(0 == (len & 0x3FF))			//ÿУ��1K����,ιһ�ι�
        {
            wdog_reload();
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief ����CRC32У��
 * @details ���㵱ǰ�������ݿ�CRC32У��
 *
 * @param  part ������
 * @param  addr ������ʼ��ַ
 * @param  len ���ݳ���
 * @return CRC32У����
 *   @retval �� 
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

