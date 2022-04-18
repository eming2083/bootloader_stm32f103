/**
  ******************************************************************************
  * @file    main.c
  * @author  eming
  * @version V1.0.0
  * @date    2022-03-21
  * @brief   ������
  ******************************************************************************
  */
  
#include "usr_cfg.h"
#include "shell_port.h"
#include "bsp.h"
#include "led.h"
#include "iap.h"
#include "sys_rst.h"
#include "ymodem.h"

static uint32_t sys_ticks_cnt = 0;
static int sys_ticks_1ms;

/**
 * @brief ϵͳ���ĺ���
 * @details 1mS�ж�һ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void SysTick_Handler(void)
{
    sys_ticks_1ms = TRUE;
    sys_ticks_cnt++;
}

/**
 * @brief ��ȡϵͳ����
 * @details ������,49������һ��
 *
 * @param ��
 * @return uint32_t,ϵͳ����
 *   @retval ��
 */
uint32_t systick_get_cnt(void)
{
    return(sys_ticks_cnt);
}

/**
 * @brief ��ʱ���к���
 * @details 10ms����һ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
static void run_timer_10ms(void)
{
    if(TRUE == shell_main())
    {
        iap_quit();
    }
    
    ymodem_task();
}

/**
 * @brief ��ʱ���к���
 * @details 100ms����һ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
static void run_timer_100ms(void)
{
    led_spark();
}

/**
 * @brief ��ʱ���к���
 * @details 1s����һ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
static void run_timer_1s(void)
{
    sys_rst_timer();
    iap_task();
}

/**
 * @brief ��Ŀ������
 * @details ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
int main(void)
{
    int sys_ticks_10ms = 0, sys_ticks_100ms = 0, sys_ticks_1s = 0;
    
    board_init();

    while (1)
    {
        if(sys_ticks_1ms == TRUE)
        {
            //1mS
            sys_ticks_1ms = FALSE;
            wdog_reload();

            //10mS
            sys_ticks_10ms++;
            if(sys_ticks_10ms >= 10)
            {
                sys_ticks_10ms = 0;
                run_timer_10ms();
            }
            
            //100mS
            sys_ticks_100ms++;
            if(sys_ticks_100ms >= 100)
            {
                sys_ticks_100ms = 0;
                run_timer_100ms();
            }
            
            //1S
            sys_ticks_1s++;
            if(sys_ticks_1s >= 1000)
            {
                sys_ticks_1s = 0;
                run_timer_1s();
            }
        }
        __wfi();
    }
}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
