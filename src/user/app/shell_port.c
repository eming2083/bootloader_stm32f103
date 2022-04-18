#include <stdio.h>
#include "usr_cfg.h"
#include "shell.h"
#include "uart.h"

#if CONFIG_SHELL_ENABLE
#define	shell_PORT_USE			    (UART1_em)
//调试接口
#define shell_data_read(a,b)	    uart_ReadData(shell_PORT_USE,a,b)
#define shell_data_write(a,b)	    uart_WriteData(shell_PORT_USE,a,b)
#define shell_data_len()		    uart_GetRcvDataNum(shell_PORT_USE)

static struct shell_input f1shell;
static uint8 shell_port_enable_flg;
//函数列表
extern void sys_date(void *arg);
extern void reboot(void);
extern int  bootm(void *arg);
extern void iap_upgrade_start(void);
extern void ymodem_start(void *arg);

/**
 * @brief 输出接口
 * @details f1shell输出接口
 *
 * @param *buf 输出的字符串
 * @param len 输出的字符串长度
 * @return 无
 *   @retval 无
 */
void f1shell_puts(const char *buf, uint16 len)
{
    shell_data_write((void *)buf, len);
}

/**
 * @brief 初始化
 * @details f1shell接口初始化
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
void shell_port_init(void)
{
    shell_init(NULL, f1shell_puts);
    shell_input_init(&f1shell, f1shell_puts);

    //注册命令
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
 * @brief shell接口使能
 * @details 如果有其他程序需要接管该通信口,使用这个函数
 *
 * @param en,TRUE使能,FALSE,去能
 * @return 无
 *   @retval 无
 */
int shell_port_enable(int en)
{
    shell_port_enable_flg = en;
}

/**
 * @brief shell输入处理
 * @details f1shell主函数
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
int  shell_main(void)
{
    uint16 len;
    char buff[32];

    if(FALSE == shell_port_enable_flg)return(FALSE);
    //===================================================================
    //检查是否有新数据
    len = shell_data_len();
    if((0 == len) || (len > sizeof(buff)))return(FALSE);
    //===================================================================
    //读取数据
    shell_data_read(buff, len);	//读取缓冲区
    shell_input(&f1shell, buff, len);
    
    return(TRUE);
}
#else
void shell_port_init(void){}
int  shell_main(void){return(FALSE);}
#endif

/**
 * @brief 错误打印函数
 * @details 错误打印输出接口
 *
 * @param *file 输出的字符串
 * @param line 输出的字符串长度
 * @return 无
 *   @retval 无
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    printk("assert failed,%s,%d\r\n", file, line);
}
/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/

