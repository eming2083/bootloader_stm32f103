/**********************************************************************************
**--------------File Info----------------------------------------------------------
** File name:			led.c
** Last modified Date:  2012-09-19
** Last Version:		1.0
** Descriptions:		ÈÎÎñº¯Êý
**
**
**---------------------------------------------------------------------------------
** Created by:			Chen Y.M
** Created date:		2011-01-07
** Version:				1.0
** Descriptions:		The original version
**
**---------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Descriptions:
**
***********************************************************************************/
#include "stm32f10x.h"
#include "usr_cfg.h"
#include "led.h"

/**********************************************************************************/
#define  PIN_LED_RUN    GPIO_Pin_2  //RUN

void led_gpio_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
    
    GPIO_ResetBits(GPIOE, PIN_LED_RUN);
}

void led_spark(void)
{
    static __IO uint32_t led1_delay_cnt = 0;
    
    if (led1_delay_cnt != 0x00)
    {
        if(led1_delay_cnt <= 5)
        {
            GPIO_ResetBits(GPIOE, PIN_LED_RUN);
        }
        else
        {
            GPIO_SetBits(GPIOE, PIN_LED_RUN);
        }
        led1_delay_cnt--;
    }
    else
    {
        led1_delay_cnt = 10;
    }
}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
