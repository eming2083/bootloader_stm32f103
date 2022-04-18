#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usr_cfg.h"
#include "uart.h"
#include "flash.h"
#include "shell.h"
#include "shell_port.h"
#include "ymodem.h"
#include "iap.h"
#include "sys_rst.h"

#if CONFIG_YMODEM_ENABLE

#define	YMODEM_PORT_USE         (UART1_em)
//�ӿ�
#define ymodem_data_read(a,b)   uart_ReadData(YMODEM_PORT_USE,a,b)
#define ymodem_data_write(a,b)  uart_WriteData(YMODEM_PORT_USE,a,b)
#define ymodem_data_len()       uart_GetRcvDataNum(YMODEM_PORT_USE)
#define ymodem_data_clear()     uart_buffer_clear(YMODEM_PORT_USE)

//������
#define ymodem_tmout_en() 		{ymodem_buff.tmout_cunt = 0;ymodem_buff.tmout_t = TRUE;}
#define ymodem_tmout_dis() 		{ymodem_buff.tmout_cunt = 0;ymodem_buff.tmout_t = FALSE;}

static ymodem_buff_stk			ymodem_buff;
static ymodem_context_stk       ymodem_context = {0};

/**
 * @brief ͨ�ų�ʱ
 * @details 1S��û���յ�������,����ǰ����յ�����
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void ymodem_timer(void)
{
    if(ymodem_buff.tmout_t == TRUE)
    {
        ymodem_buff.tmout_cunt++;
        if(ymodem_buff.tmout_cunt > 100)
        {
            ymodem_buff.tmout_cunt = 0;
            ymodem_buff.tmout_t = FALSE;
            ymodem_buff.num = 0;
        }
    }
}

/**
 * @brief ͨ�����ݷ���
 * @details 1S��û���յ�������,����ǰ����յ�����
 *
 * @param p,�����͵�����ָ��
 * @param len,�����͵����ݳ���
 * @return ��
 *   @retval ��
 */
static void ymodem_send_data(uint8 dat)
{
    ymodem_data_clear();
    ymodem_data_write(&dat, 1);
    ymodem_context.comm_tmout = 0;
}

/**
 * @brief   ������յ����� CRC-16.
 * @param   *data:  Ҫ��������ݵ�����.
 * @param   length: ���ݵĴ�С��128�ֽڻ�1024�ֽ�.
 * @return  status: ����CRC.
 */
static uint16_t ymodem_calc_crc(uint8 *data, uint16 length)
{
    uint16 crc = 0u;
    while (length)
    {
        length--;
        crc = crc ^ ((uint16) * data++ << 8u);
        for (uint8 i = 0; i < 8; i++)
        {
            if (crc & 0x8000u)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/**
 * @brief �������ݴ�����
 * @details 
 *
 * @param p,����ָ��
 * @param len,���ݳ���
 * @return ��
 *   @retval ��
 */
static void ymodem_data_handler(uint8 *p, uint16 len)
{
    uint16 dat_len, crc, str_len;
    ymodem_frame_stk *stkp;
    
    stkp = (ymodem_frame_stk*)p;

    switch(stkp->stk.h1)
    {
    case YMODEM_SOH:
    case YMODEM_STX:
        if(stkp->stk.h1 == YMODEM_SOH)
        {
            dat_len = YMODEM_PACKET_128_SIZE;
        }
        else
        {
            dat_len = YMODEM_PACKET_1024_SIZE;
        }
        //CRCУ��
        crc = (stkp->stk.dat[dat_len] << 8) + stkp->stk.dat[dat_len + 1];
        if(crc != ymodem_calc_crc(stkp->stk.dat, dat_len))return;
        
        //������
        if(ymodem_context.pkgs_num == 0)
        {
            //��һ����,��ȡ�ļ���Ϣ
            str_len = strlen((char*)stkp->stk.dat);
            ymodem_context.file_size = atoi((char*)&stkp->stk.dat[str_len + 1]);
        }
        else
        {
            //д������
            if(ymodem_context.file_offset < ymodem_context.file_size)
            {
                flash_write_without_erase(ymodem_context.flash_partition, ymodem_context.file_offset, stkp->stk.dat, dat_len);
                ymodem_context.file_offset += dat_len;
            }
            else
            {
                //�հ�
                
            }
        }
        ymodem_context.pkgs_num++;
        //����Ӧ��
        ymodem_send_data(YMODEM_ACK);        
        break;
    
    case YMODEM_EOT:
        if(ymodem_context.eot_num == 0)
        {
            ymodem_context.eot_num++;
            ymodem_send_data(YMODEM_NAK); 
        }
        else
        {
            ymodem_send_data(YMODEM_ACK);
            //�������
            if(ymodem_context.flash_partition == flash_bak)
            {
                iap_upgrade_start();
            }
            reboot();
            
            //�ָ��Ĵ���
            ymodem_context.task_ps = 1;
            ymodem_context.pkgs_num = 0;
            ymodem_context.eot_num = 0;
            ymodem_context.file_offset = 0;
        }
        break;
    }
}
    
/**
 * @brief ȡ֡����
 * @details ������������ȡ��������������֡
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
static void ymodem_frame(uint8 dat)
{
    switch(ymodem_buff.num)
    {
    case 0:								//�ж�֡ͷ
        if((YMODEM_SOH == dat) || (YMODEM_STX == dat))
        {
            ymodem_buff.buff[ymodem_buff.num++] = dat;
            ymodem_tmout_en();
        }
        else if(YMODEM_EOT == dat)
        {
            ymodem_buff.buff[ymodem_buff.num++] = dat;
            ymodem_data_handler(ymodem_buff.buff, 1);
            ymodem_buff.num = 0;
            ymodem_tmout_dis();	
        }
        else
        {
            ymodem_buff.num = 0;
            ymodem_tmout_dis();	
        }
        break;

    case 1:
        ymodem_buff.buff[ymodem_buff.num++] = dat;
        ymodem_tmout_en();
        break;

    case 2:
        if(((dat + ymodem_buff.buff[1]) & 0xFF) == 0xFF)
        {
            ymodem_buff.buff[ymodem_buff.num++] = dat;
            ymodem_tmout_en();
            if(ymodem_buff.buff[0] == YMODEM_SOH)
            {
                ymodem_buff.len = YMODEM_PACKET_128_SIZE + 5;
            }
            else
            {
                ymodem_buff.len = YMODEM_PACKET_1024_SIZE + 5;
            }
        }
        else
        {
            ymodem_buff.num = 0;
            ymodem_tmout_dis();
        }
        break;

    default:
        if(ymodem_buff.num < ymodem_buff.len)
        {
            ymodem_buff.buff[ymodem_buff.num++] = dat;
            ymodem_tmout_en();
            if(ymodem_buff.num == ymodem_buff.len)
            {
                ymodem_data_handler(ymodem_buff.buff,ymodem_buff.len);
                ymodem_buff.num = 0;
                ymodem_tmout_dis();
            }			
        }
        else
        {
            ymodem_buff.num = 0;
            ymodem_tmout_dis();
        }
        break;
    }
}

/**
 * @brief   ymodem������.
 * @param   void.
 * @return  void.
 */
void ymodem_task(void)
{
    uint16 len;
    uint8 buff[32];
    
    //�������ݴ���
    if(ymodem_context.task_ps > 0)
    {
        //��ʱ��ʱ��
        ymodem_timer();
        //�������ݴ���
        if(ymodem_data_len() > 0)
        {
            //��ȡ����
            len = ymodem_data_read(buff, sizeof(buff));
            for(int i = 0; i < len; i++)
            {
                ymodem_frame(buff[i]);
            }
        }        
    }
    
    //�������̿���
    switch(ymodem_context.task_ps)
    {
    case 0:
    default:
        break;
    
    case 1:
        ymodem_send_data(YMODEM_C);
        ymodem_context.comm_tmout = 0;
        ymodem_context.task_ps = 2;
        break;
    
    case 2:
        //����֡��ʱ
        ymodem_context.comm_tmout++;
        if(ymodem_context.comm_tmout > 100)
        {
            ymodem_context.task_ps = 1;
        }
        break;

    }
}

#if CONFIG_SHELL_ENABLE
/**
 * @brief   ymodem��ʼ.
 * @param   void.
 * @return  void.
 */
void ymodem_start(void *arg)
{
    static const char tips[] = "ymodem [app, patch]\r\n";
    char *argv[2];
    uint32_t entry;
    int argc = cmdline_strtok((char *)arg, argv, 2);
    
    if(argc < 2)
    {
        printl((char *)tips, sizeof(tips) - 1);
        return;
    }
    
    if(strcmp(argv[1], "app") == 0)
    {
        ymodem_context.flash_partition = flash_app;
        flash_partition_erase(flash_app, 0, FLASH_APP_SIZE);
    }
    else if(strcmp(argv[1], "patch") == 0)
    {
        ymodem_context.flash_partition = flash_bak;
        flash_partition_erase(flash_bak, 0, FLASH_BAK_SIZE);
    }
    ymodem_context.task_ps = 1;
    ymodem_context.pkgs_num = 0;
    ymodem_context.eot_num = 0;
    ymodem_context.file_offset = 0;
    shell_port_enable(FALSE);
}
#endif

#else
void ymodem_task(void) {}
#endif
/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/

