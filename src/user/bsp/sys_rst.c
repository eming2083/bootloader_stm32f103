#include <ustdio.h>
#include "usr_cfg.h"
#include "type.h"
#include "sys_rst.h"

/*************************************************************/
static Sys_Boot_stk		Sys_Boot;
extern void SystemReset(void);


void sys_rst_init(uint32_t tm)
{
    Sys_Boot.tm_max = tm;
    Sys_Boot.tm = 0;
    Sys_Boot.flg = TRUE;
    printk("system will reset after %dS!\r\n", tm);
}

void sys_rst_timer(void)
{
    if(TRUE == Sys_Boot.flg)
    {
        Sys_Boot.tm++;
        if(Sys_Boot.tm > Sys_Boot.tm_max)
        {
            SystemReset();
        }
    }
}

void reboot(void)
{
    sys_rst_init(1);
}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
