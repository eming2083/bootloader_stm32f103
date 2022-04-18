/**
  ******************************************************************************
  * @file    bsp.c
  * @author  eming
  * @version V1.0.0
  * @date    2022-03-21
  * @brief   硬件模块初始化.
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
 * @brief GPIO配置
 * @details 打开GPIO时钟,禁用JTAG引脚,初始化其他模块
 *
 * @param 无
 * @return 无
 *   @retval 无
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

    /// 禁用Jtag引脚
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    /// LED
    led_gpio_init();
}

/**
 * @brief 看门狗配置
 * @details 使能看门,溢出周期设置为3.2S
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
void IWDG_Configuration (void)
{
    /// 因为独立看门狗使用的是LSI,所以最好程序启动的时候,使时钟源稳定
    /// LSI的启动
    RCC_LSICmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET); ///< 等待直到LSI稳定

    /// 启动独立看门狗
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);      ///< 访问之前要首先使能寄存器写

    IWDG_SetPrescaler(IWDG_Prescaler_128);             ///< 128分频 一个周期3.2ms
    IWDG_SetReload(1000);                              ///< 最长12位 [0,4096] 1000*3.2=3.2S
    /// Reload IWDG counter
    IWDG_ReloadCounter();
    IWDG_Enable();
}

/**
 * @brief MCU复位函数
 * @details 复位系统,不会返回
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
__asm void SystemReset(void)
{
    MOV R0, #1             //;
    MSR FAULTMASK, R0      //; 清除FAULTMASK 禁止一切中断产生
    LDR R0, = 0xE000ED0C   //;
    LDR R1, = 0x05FA0004   //;
    STR R1, [R0]           //; 系统软件复位

deadloop
    B deadloop             //; 死循环使程序运行不到下面的代码
}

/**
 * @brief SysTick配置
 * @details 配置系统滴答时钟
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
void SysTick_Configuration(void)
{
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(SysTick_IRQn, 0x0); 
}

/**
 * @brief 读取SysTick
 * @details 读取SysTick滴答定时器计数值
 *
 * @param 无
 * @return 定时器计数值
 *   @retval SysTick->VAL
 */
unsigned long SysTick_GetCounter(void)
{
    return(SysTick->VAL & 0x00FFFFFF);
}

/**
 * @brief 获取CPUID
 * @details 获取CPU唯一ID
 *
 * @param *id 读取的96bit数据,12字节数组
 * @return 无
 *   @retval 无
 */
void sys_get_cpuid(uint8 *id)
{
    U32_U8 hwid[3];
    /// 获取CPU唯一ID
    hwid[0].d32 = *(vu32 *)(0x1ffff7e8);
    hwid[1].d32 = *(vu32 *)(0x1ffff7ec);
    hwid[2].d32 = *(vu32 *)(0x1ffff7f0);
    memcpy(id, hwid, sizeof(hwid));
}

/**
 * @brief 喂狗
 * @details 重置看门狗计数器
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
void wdog_reload(void)
{
#if (WDOG_ENABLE == TRUE)
    IWDG_ReloadCounter();
#endif
}

/**
 * @brief 板级硬件初始化
 * @details GPIO,UART,SysTick等初始化
 *
 * @param 无
 * @return 无
 *   @retval 无
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
 * @brief 板级硬件反初始化
 * @details 主要是关闭串口
 *
 * @param 无
 * @return 无
 *   @retval 无
 */
void board_deinit(void)
{
    uart_DeInit();
}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
