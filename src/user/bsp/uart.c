/**
  ******************************************************************************
  * @file    uart.c
  * @author  eming
  * @version V1.0.0
  * @date    2021-09-19
  * @brief   ���ڳ�ʼ��.
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stm32f10x.h"
#include "usr_cfg.h"
#include "uart.h"

/**********************************************************************************/

/**********************************************************************************/
static uint8_t UartRcvbuff1[UART1_RX_BUFFLEN];		///< ����1���ջ���
static uint8_t UartSndbuff1[UART1_TX_BUFFLEN];		///< ����1���ͻ���
static stm32_serial_stk serial_dev[UART_NUM];

/**
 * @brief ����Ӳ����ʼ��
 * @details ʹ�ܴ���ʱ��,����,�жϽ���ģʽ
 *
 * @param UART_enum ���ں�
 * @return ��
 *   @retval ��
 */
static void uart_hw_init(UART_enum num)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Configure one bit for preemption priority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    switch(num)
    {
    case UART1_em:
        /* Enable clock                                                           */
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

        /* Configure USART1 Rx (PA10) as input floating                           */
        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        /* Configure USART1 Tx (PA9) as alternate function push-pull              */
        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        USART_InitStructure.USART_BaudRate            = 115200;
        USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits            = USART_StopBits_1;
        USART_InitStructure.USART_Parity              = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
        USART_Init(USART1, &USART_InitStructure);

        /* Enable the USART1 Interrupt */
        USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);      //ʹ�ܽ����ж�
        USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);      //ʹ�ܿ����ж�
        //USART_ITConfig(USART1,USART_IT_TXE,ENABLE);         //ʹ�ܷ����ж�

        /* Enable USART1                      */
        USART_Cmd(USART1, ENABLE);

        /* Enable the UART1 Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel 						= USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority 	= 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority 			= 15;
        NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        break;

    default:
        break;
    }
}

/**
 * @brief ��ʼ�����ڽ��պͷ���ringBuff
 * @details ���ͺͽ��վ����û��λ�����
 *
 * @param UART_enum ���ں�
 * @return ��
 *   @retval ��
 */
void uart_buffer_clear(UART_enum num)
{
    switch(num)
    {
    case UART1_em:
        serial_dev[0].rx_eof  = FALSE;
        serial_dev[0].tx_busy = FALSE;
        ring_buffer_init(&serial_dev[0].rx_buff, UartRcvbuff1, sizeof(UartRcvbuff1));
        ring_buffer_init(&serial_dev[0].tx_buff, UartSndbuff1, sizeof(UartSndbuff1));
        break;

    default:
        break;
    }
}

/**
 * @brief ���ô��ڲ�����
 * @details ���ô��ڲ�����,ͬʱ�����³�ʼ�����պͷ��ͻ�����
 *
 * @param UART_enum ���ں�
 * @param band ������
 * @return ��
 *   @retval ��
 */
void uart_band_set(UART_enum num, int band)
{
    USART_InitTypeDef USART_InitStructure;
    stm32_serial_stk *ch;

    ch = &serial_dev[num];

    /* Enable DISABLE                     */
    USART_Cmd(ch->dev, DISABLE);

    USART_InitStructure.USART_BaudRate            = band;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(ch->dev, &USART_InitStructure);

    /* Enable USART                      */
    USART_Cmd(ch->dev, ENABLE);

    uart_buffer_clear(num);
}

/**
 * @brief �򿪴���
 * @details Ӳ���ͻ�������ʼ��
 *
 * @param UART_enum ���ں�
 * @return ��
 *   @retval ��
 */
static void uart_open(UART_enum num)
{
    uart_hw_init(num);
    uart_buffer_clear(num);
}

/**
 * @brief �رմ���
 * @details Ӳ������ʼ��
 *
 * @param UART_enum ���ں�
 * @return ��
 *   @retval ��
 */
void uart_close(UART_enum num)
{
    switch(num)
    {
    case UART1_em:
        USART_DeInit(USART1);
        break;

    }
    
}

/**
 * @brief �����жϷ������
 * @details �����ڷ��ͺͽ����ж�
 *
 * @param  UART_enum ���ں�
 * @return ��
 *   @retval ��
 */
static void uart_ISR(UART_enum num)
{
    stm32_serial_stk *ch;
    uint8_t dat;

    ch = &serial_dev[num];
    /* �����ж� */
    if(SET == USART_GetITStatus(ch->dev, USART_IT_RXNE))
    {
        USART_ClearITPendingBit(ch->dev, USART_IT_RXNE);
        dat = USART_ReceiveData(ch->dev);
        ring_buffer_save_isr(&ch->rx_buff, dat);
    }
    /* ���տ����ж� */
    if(SET == USART_GetITStatus(ch->dev, USART_IT_IDLE))
    {
        dat = USART_ReceiveData(ch->dev);
        ch->rx_eof = TRUE;
    }
    /* �����ж� */
    if(SET == USART_GetITStatus(ch->dev, USART_IT_TXE))
    {
        if (FALSE == ring_buffer_isEmpty(&ch->tx_buff))
        {
            /* read a character */
            ring_buffer_read_isr(&ch->tx_buff, &dat, 1);
            ch->dev->DR = dat & 0x1FF;
        }
        else
        {
            USART_ITConfig(ch->dev, USART_IT_TXE, DISABLE);
        }
    }
    /* ��������ж� */
    if(SET == USART_GetITStatus(ch->dev, USART_IT_TC))
    {
        USART_GetFlagStatus(ch->dev, USART_FLAG_TC);
        //USART_ITConfig(ch->dev, USART_IT_TC, DISABLE);
    }
}

/**
 * @brief �����жϷ������
 * @details �ж�������������
 *
 * @param  ��
 * @return ��
 *   @retval ��
 */
void USART1_IRQHandler(void)
{
    uart_ISR(UART1_em);
}

void USART2_IRQHandler(void)
{
    
}

void USART3_IRQHandler(void)
{
    
}

void UART4_IRQHandler(void)
{
    
}

void UART5_IRQHandler(void)
{
    
}

/**
 * @brief ��ȡ��������
 * @details ��ȡ���ڽ��յ�������
 *
 * @param  UART_enum ���ں�
 * @param  buffer ����ָ��
 * @param  size ��ȡ����������
 * @return ��ȡ���������ֽ���
 *   @retval uint16_t
 */
uint16_t uart_ReadData(UART_enum num, void *buffer, uint16_t size)
{
    stm32_serial_stk *ch;
    uint8_t *ptr;
    uint16_t ret_len;

    ptr = (uint8_t *)buffer;
    ch = &serial_dev[num];
    ret_len = ring_buffer_read(&ch->rx_buff, ptr, size);
    
    return (ret_len);
}

/**
 * @brief ��ȡ���������ֽ���
 * @details ��ȡ��ǰ���ջ����������ֽ���
 *
 * @param  UART_enum ���ں�
 * @return ��ȡ���������ֽ���
 *   @retval uint16_t
 */
uint16_t uart_GetRcvDataNum(UART_enum num)
{
    stm32_serial_stk *ch;
    uint16_t ret_len;

    ch = &serial_dev[num];
    ret_len = ring_buffer_data_num(&ch->rx_buff);

    return (ret_len);
}

/**
 * @brief �Ƿ���յ��ļ�β
 * @details ��Ӳ�����������ж�ʱ
 *
 * @param  UART_enum ���ں�
 * @return �������Ƿ���ֹ
 *   @retval TRUE
 *   @retval FALSE
 */
int uart_GetRcvDataEOF(UART_enum num)
{
    stm32_serial_stk *ch;
    int  eof;

    ch = &serial_dev[num];
    eof = ch->rx_eof;
    ch->rx_eof = FALSE;

    return(eof);
}

/**
 * @brief ��������
 * @details ���ͻ�����д�����ݲ���������
 *
 * @param  UART_enum ���ں�
 * @param  buffer д�������ָ��
 * @param  size  д��������ֽ���
 * @return �Ѿ�д���������
 *   @retval ��
 */
uint16_t uart_WriteData(UART_enum num, const void *buffer, uint16_t size)
{
    stm32_serial_stk *ch;
    uint8_t *ptr;

    ch = &serial_dev[num];
    ptr = (uint8_t *)buffer;

    /// �����ֶγ���
    USART_ITConfig(ch->dev, USART_IT_TXE, DISABLE);	///< ��ֹ�����ж�,ȷ������Ĵ������жϻ���
    while (size--)
    {
        ring_buffer_save(&ch->tx_buff, *(ptr++));
    }
    USART_ITConfig(ch->dev, USART_IT_TXE, ENABLE); 	///< ʹ�ܷ����ж�
    
    return (ptr - (uint8_t *)buffer);
}

/**
 * @brief ���ڳ�ʼ��
 * @details ���ڳ�ʼ����ں���
 *
 * @param ��
 * @return ��
 *   @retval ��
 */
void uart_Init(void)
{
    serial_dev[0].dev = USART1;

    uart_open(UART1_em);
}

void uart_DeInit(void)
{
    uart_close(UART1_em);
}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
