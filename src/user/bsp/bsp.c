/**
  ******************************************************************************
  * @file    bsp.c
  * @author  eming
  * @version V1.0.0
  * @date    2022-03-21
  * @brief   Ӳ��ģ���ʼ��.
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/
#include <ustdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "usr_cfg.h"
#include "led.h"
#include "uart.h"
#include "sys_time.h"
#include "shell_port.h"


/**
 * @brief GPIO����
 * @details ��GPIOʱ��,����JTAG����,��ʼ������ģ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void GPIO_Configuration (void)
{
    /// Enable clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

    /// ����Jtag����
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    /// LED
    led_gpio_init();
}

/**
 * @brief ���Ź�����
 * @details ʹ�ܿ���,�����������Ϊ3.2S
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void IWDG_Configuration (void)
{
    /// ��Ϊ�������Ź�ʹ�õ���LSI,������ó���������ʱ��,ʹʱ��Դ�ȶ�
    /// LSI������
    RCC_LSICmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET); ///< �ȴ�ֱ��LSI�ȶ�

    /// �����������Ź�
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);      ///< ����֮ǰҪ����ʹ�ܼĴ���д

    IWDG_SetPrescaler(IWDG_Prescaler_128);             ///< 128��Ƶ һ������3.2ms
    IWDG_SetReload(1000);                              ///< �12λ [0,4096] 1000*3.2=3.2S
    /// Reload IWDG counter
    IWDG_ReloadCounter();
    IWDG_Enable();
}

/**
 * @brief MCU��λ����
 * @details ��λϵͳ,���᷵��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
__asm void SystemReset(void)
{
    MOV R0, #1             //;
    MSR FAULTMASK, R0      //; ���FAULTMASK ��ֹһ���жϲ���
    LDR R0, = 0xE000ED0C   //;
    LDR R1, = 0x05FA0004   //;
    STR R1, [R0]           //; ϵͳ�����λ

deadloop
    B deadloop             //; ��ѭ��ʹ�������в�������Ĵ���
}

/**
 * @brief SysTick����
 * @details ����ϵͳ�δ�ʱ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void SysTick_Configuration(void)
{
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(SysTick_IRQn, 0x0); 
}

/**
 * @brief ��ȡSysTick
 * @details ��ȡSysTick�δ�ʱ������ֵ
 *
 * @param ��
 * @return ��ʱ������ֵ
 *   @retval SysTick->VAL
 */
unsigned long SysTick_GetCounter(void)
{
    return(SysTick->VAL & 0x00FFFFFF);
}

/**
 * @brief ��ȡCPUID
 * @details ��ȡCPUΨһID
 *
 * @param *id ��ȡ��96bit����,12�ֽ�����
 * @return ��
 *   @retval ��
 */
void sys_get_cpuid(uint8 *id)
{
    U32_U8 hwid[3];
    /// ��ȡCPUΨһID
    hwid[0].d32 = *(vu32 *)(0x1ffff7e8);
    hwid[1].d32 = *(vu32 *)(0x1ffff7ec);
    hwid[2].d32 = *(vu32 *)(0x1ffff7f0);
    memcpy(id, hwid, sizeof(hwid));
}

/**
 * @brief ι��
 * @details ���ÿ��Ź�������
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void wdog_reload(void)
{
#if (WDOG_ENABLE == TRUE)
    IWDG_ReloadCounter();
#endif
}

/**
 * @brief �弶Ӳ����ʼ��
 * @details GPIO,UART,SysTick�ȳ�ʼ��
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void board_init(void)
{
    GPIO_Configuration();
    uart_Init();
    shell_port_init();
    sys_time_init();
    SysTick_Configuration();

#if CONFIG_WDOG_ENABLE
    IWDG_Configuration();
    #warning ("used internal watchdog!")
#else
    #warning ("unused internal watchdog!")
    printk("unused onchip watchdog!\r\n");
#endif
}

/**
 * @brief �弶Ӳ������ʼ��
 * @details ��Ҫ�ǹرմ���
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void board_deinit(void)
{
    uart_DeInit();
}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
