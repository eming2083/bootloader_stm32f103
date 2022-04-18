#include <stdint.h>
#include <ustdio.h>
#include <time.h>
#include "stm32f10x.h"
#include "usr_cfg.h"
#include "type.h"
#include "sys_time.h"

/*************************************************************/
#define BKP_MEM_MAGIC       (0xA5A5)

/*******************************************************************************
* Function Name  : RTC_Configuration
* Description    : Configures the RTC.
* Input          : None
* Output         : None
* Return         : 0 reday,-1 error.
*******************************************************************************/
static int RTC_Configuration(void)
{
    u32 count = 0x200000;

    /* Reset Backup Domain */
    BKP_DeInit();

    /* Enable LSE */
    RCC_LSEConfig(RCC_LSE_ON);
    /* Wait till LSE is ready */
    while ( (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) && (--count) );
    if ( count == 0 )
    {
        return -1;
    }

    /* Select LSE as RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

    /* Enable RTC Clock */
    RCC_RTCCLKCmd(ENABLE);

    /* Wait for RTC registers synchronization */
    RTC_WaitForSynchro();

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    /* Set RTC prescaler: set RTC period to 1sec */
    RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    RTC_SetCounter(SYS_ORIGIN_TIME);                //��ʼֵ�趨

    RTC_WaitForLastTask();

    RTC_SetAlarm(60);	                           //����ֵ�趨Ϊ60s

    RTC_WaitForLastTask();                        //�ȴ������������

    PWR_BackupAccessCmd(DISABLE);

    return 0;
}


void rtc_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//ʹ��PWR��BKP����ʱ��   
	PWR_BackupAccessCmd(ENABLE);	//ʹ�ܺ󱸼Ĵ�������  
	if (BKP_ReadBackupRegister(BKP_DR1) != BKP_MEM_MAGIC)	
    {
        printk("rtc is not configured\r\n");

        if ( RTC_Configuration() != 0)
        {
            printk("rtc configure fail...\r\n");
            return ;
        }
        BKP_WriteBackupRegister(BKP_DR1, BKP_MEM_MAGIC);
    }
    else
    {
        /* Wait for RTC registers synchronization */
        RTC_WaitForSynchro();
    }

    //����RTC�ж�
    /*
    {
        NVIC_InitTypeDef NVIC_InitStructure;

        RTC_ITConfig(RTC_IT_SEC | RTC_IT_ALR, ENABLE);	//��RTC�����жϺ������ж�

        //��������
        PWR_BackupAccessCmd(ENABLE);
        RTC_EnterConfigMode();
        RTC_WaitForLastTask();
        RTC_SetAlarm(60 + RTC_GetCounter());	 //�����´�����Ϊ60s��
        RTC_WaitForLastTask();
        RTC_ExitConfigMode();
        PWR_BackupAccessCmd(DISABLE);

        //�ж�����
        NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;		//RTCȫ���жϵ��ж�����
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }
    */

    return;
}


void RTC_IRQHandler()
{
    if(RTC_GetITStatus(RTC_IT_ALR) != RESET) //�Ƿ������жϷ���
    {

        printk("THE ALARM  READY = %d \r\n", RTC_GetCounter()); //�����ʱ������
        RTC_ClearITPendingBit(RTC_IT_ALR);

        PWR_BackupAccessCmd(ENABLE);
        RTC_EnterConfigMode();
        RTC_WaitForLastTask();
        RTC_SetAlarm(60 + RTC_GetCounter());	 //�����´�����Ϊ40s��
        RTC_WaitForLastTask();
        RTC_ExitConfigMode();
        PWR_BackupAccessCmd(DISABLE);
    }
    if(RTC_GetITStatus(RTC_IT_SEC) != RESET) //�Ƿ����жϷ���
    {
        //printk("Time is = %d \r\n",RTC_GetCounter()); //�����ʱ������
    }
    RTC_ClearITPendingBit(RTC_IT_SEC | RTC_IT_OW); //������жϱ�־λ�����λ
    RTC_WaitForLastTask();
}


int rtc_set_utc(time_t utc_s)
{
    /* Enable PWR and BKP clocks */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

    /* Allow access to BKP Domain */
    PWR_BackupAccessCmd(ENABLE);
    
    BKP_WriteBackupRegister(BKP_DR1, BKP_MEM_MAGIC);

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    /* Change the current time */
    RTC_SetCounter((uint32_t)utc_s);

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    PWR_BackupAccessCmd(DISABLE);
    
    return(TRUE);
}

int rtc_get_utc(time_t *utc_s)
{
    time_t sec;

    sec = RTC_GetCounter();

    if(sec > SYS_ORIGIN_TIME)
    {
        *utc_s = sec;
        return(TRUE);
    }
    return (FALSE);
}


/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/

