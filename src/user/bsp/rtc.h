#ifndef __STM_RTC_H__
#define __STM_RTC_H__
/*******************************************************************************************************/
#include <time.h>

/*******************************************************************************************************/
void rtc_init(void);
int rtc_set_utc(time_t utc);
int rtc_get_utc(time_t *utc);

#endif
/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
