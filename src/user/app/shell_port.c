#include <stdio.h>
#include "usr_cfg.h"
#include "shell.h"
#include "uart.h"

#if CONFIG_SHELL_ENABLE
#define	shell_PORT_USE			    (UART1_em)
//���Խӿ�
#define shell_data_read(a,b)	    uart_ReadData(shell_PORT_USE,a,b)
#define shell_data_write(a,b)	    uart_WriteData(shell_PORT_USE,a,b)
#define shell_data_len()		    uart_GetRcvDataNum(shell_PORT_USE)

static struct shell_input f1shell;
static uint8 shell_port_enable_flg;
//�����б�
extern void sys_date(void *arg);
extern void reboot(void);
extern int  bootm(void *arg);
extern void iap_upgrade_start(void);
extern void ymodem_start(void *arg);

/**
 * @brief ����ӿ�
 * @details f1shell����ӿ�
 *
 * @param *buf ������ַ���
 * @param len ������ַ�������
 * @return ��
 *   @retval ��
 */
void f1shell_puts(const char *buf, uint16 len)
{
    shell_data_write((void *)buf, len);
}

/**
 * @brief ��ʼ��
 * @details f1shell�ӿڳ�ʼ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void shell_port_init(void)
{
    shell_init(NULL, f1shell_puts);
    shell_input_init(&f1shell, f1shell_puts);

    //ע������
    shell_register_command("date", (cmd_fn_t)sys_date);
    shell_register_command("reboot", (cmd_fn_t)reboot);
    shell_register_command("bootm", (cmd_fn_t)bootm);
    shell_register_command("iap-start", (cmd_fn_t)iap_upgrade_start);
    
#if CONFIG_YMODEM_ENABLE
    shell_register_command("ymodem", (cmd_fn_t)ymodem_start);
#endif
    
    shell_port_enable_flg = TRUE;
    //printk("Shell is running.\r\n");
}

/**
 * @brief shell�ӿ�ʹ��
 * @details ���������������Ҫ�ӹܸ�ͨ�ſ�,ʹ���������
 *
 * @param en,TRUEʹ��,FALSE,ȥ��
 * @return ��
 *   @retval ��
 */
int shell_port_enable(int en)
{
    shell_port_enable_flg = en;
}

/**
 * @brief shell���봦��
 * @details f1shell������
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
int  shell_main(void)
{
    uint16 len;
    char buff[32];

    if(FALSE == shell_port_enable_flg)return(FALSE);
    //===================================================================
    //����Ƿ���������
    len = shell_data_len();
    if((0 == len) || (len > sizeof(buff)))return(FALSE);
    //===================================================================
    //��ȡ����
    shell_data_read(buff, len);	//��ȡ������
    shell_input(&f1shell, buff, len);
    
    return(TRUE);
}
#else
void shell_port_init(void){}
int  shell_main(void){return(FALSE);}
#endif

/**
 * @brief �����ӡ����
 * @details �����ӡ����ӿ�
 *
 * @param *file ������ַ���
 * @param line ������ַ�������
 * @return ��
 *   @retval ��
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    printk("assert failed,%s,%d\r\n", file, line);
}
/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/

