#ifndef __TARGET_H__
#define __TARGET_H__
/********************************************************************************************************/
void  SystemReset(void);
unsigned long    SysTick_GetCounter(void);
void  board_init(void);
void  board_deinit(void);
void  wdog_reload(void);
/********************************************************************************************************/
#endif
