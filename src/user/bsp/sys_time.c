#include <ustdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "usr_cfg.h"
#include "type.h"
#include "rtc.h"
#include "math.h"

/*******************************************************************************************************/

void sys_time_init(void)
{
    rtc_init();
}

struct tm *sys_time_rtc_get(void)
{
    time_t utc_s;
    time_t loc_s;
    struct tm *t = NULL;

    if(TRUE == rtc_get_utc(&utc_s))
    {
        loc_s = utc_s + 8 * 3600;
        t = localtime(&loc_s);
    }
    return(t);
}

uint32_t sys_time_utc_get(void)
{
    time_t utc_s;

    if(TRUE == rtc_get_utc(&utc_s))
    {
        return(utc_s);
    }
    else
    {
        return(0);
    }
}

void sys_time_utc_set(uint32_t ts)
{
    rtc_set_utc(ts);
}


/*******************************************************************************************************
**                            Shell
********************************************************************************************************/
#if CONFIG_SHELL_ENABLE

#include "shell.h"
void sys_time_print(void)
{
    struct tm *t;

    t = sys_time_rtc_get();
    if(NULL != t)
    {
        printk("TIME:time is %d-%02d-%02d %02d:%02d:%02d\r\n",	\
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, 	\
               t->tm_hour, t->tm_min, t->tm_sec);
    }
    else
    {
        printk("TIME:time is error.\r\n");
    }
}

void sys_date(void *arg)
{
    static const char tips[] = "date [y[0-99] m[1-12] d[1-31] h[0-23] m[0-59] s[0-59]]\r\n";
    int argc;
    int argv[6];
    struct tm rtc;
    uint32 utc_s;

    argc = cmdline_param((char *)arg, argv, 6);
    if (argc < 1)
    {
        printl((char *)tips, sizeof(tips) - 1);
    }
    else
    {
        rtc.tm_year = argv[0] + 100;
        rtc.tm_mon  = argv[1] - 1;
        rtc.tm_mday = argv[2];
        rtc.tm_hour = argv[3];
        rtc.tm_min  = argv[4];
        rtc.tm_sec  = argv[5];

        utc_s = mktime(&rtc) - 8 * 3600;
        sys_time_utc_set(utc_s);
    }
    sys_time_print();
}
#endif

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/



