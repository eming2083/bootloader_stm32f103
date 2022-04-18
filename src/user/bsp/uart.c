/**
  ******************************************************************************
  * @file    uart.c
  * @author  eming
  * @version V1.0.0
  * @date    2021-09-19
  * @brief   串口初始化.
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stm32f10x.h"
#include "usr_cfg.h"
#include "uart.h"

/**********************************************************************************/

/**********************************************************************************/
static uint8_t UartRcvbuff1[UART1_RX_BUFFLEN];		///< 串口1接收缓冲
static uint8_t UartSndbuff1[UART1_TX_BUFFLEN];		///< 串口1发送缓冲
static stm32_serial_stk serial_dev[UART_NUM];

/**
 * @brief 串口硬件初始化
 * @details 使能串口时钟,引脚,中断接收模式
 *
 * @param UART_enum 串口号
 * @return 无
 *   @retval 无
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
        USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);      //使能接收中断
        USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);      //使能空闲中断
        //USART_ITConfig(USART1,USART_IT_TXE,ENABLE);         //使能发送中断

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
 * @brief 初始化串口接收和发送ringBuff
 * @details 发送和接收均采用环形缓冲区
 *
 * @param UART_enum 串口号
 * @return 无
 *   @retval 无
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
 * @brief 设置串口波特率
 * @details 设置串口波特率,同时会重新初始化接收和发送缓冲区
 *
 * @param UART_enum 串口号
 * @param band 波特率
 * @return 无
 *   @retval 无
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
 * @brief 打开串口
 * @details 硬件和缓冲区初始化
 *
 * @param UART_enum 串口号
 * @return 无
 *   @retval 无
 */
static void uart_open(UART_enum num)
{
    uart_hw_init(num);
    uart_buffer_clear(num);
}

/**
 * @brief 关闭串口
 * @details 硬件反初始化
 *
 * @param UART_enum 串口号
 * @return 无
 *   @retval 无
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
 * @brief 串口中断服务程序
 * @details 处理串口发送和接收中断
 *
 * @param  UART_enum 串口号
 * @return 无
 *   @retval 无
 */
static void uart_ISR(UART_enum num)
{
    stm32_serial_stk *ch;
    uint8_t dat;

    ch = &serial_dev[num];
    /* 接收中断 */
    if(SET == USART_GetITStatus(ch->dev, USART_IT_RXNE))
    {
        USART_ClearITPendingBit(ch->dev, USART_IT_RXNE);
        dat = USART_ReceiveData(ch->dev);
        ring_buffer_save_isr(&ch->rx_buff, dat);
    }
    /* 接收空闲中断 */
    if(SET == USART_GetITStatus(ch->dev, USART_IT_IDLE))
    {
        dat = USART_ReceiveData(ch->dev);
        ch->rx_eof = TRUE;
    }
    /* 发送中断 */
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
    /* 发送完成中断 */
    if(SET == USART_GetITStatus(ch->dev, USART_IT_TC))
    {
        USART_GetFlagStatus(ch->dev, USART_FLAG_TC);
        //USART_ITConfig(ch->dev, USART_IT_TC, DISABLE);
    }
}

/**
 * @brief 串口中断服务程序
 * @details 中断向量表调用入口
 *
 * @param  无
 * @return 无
 *   @retval 无
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
 * @brief 读取串口数据
 * @details 读取串口接收到的数据
 *
 * @param  UART_enum 串口号
 * @param  buffer 数据指针
 * @param  size 读取的数据数量
 * @return 读取到的数据字节数
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
 * @brief 读取串口数据字节数
 * @details 获取当前接收缓冲区数据字节数
 *
 * @param  UART_enum 串口号
 * @return 读取到的数据字节数
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
 * @brief 是否接收到文件尾
 * @details 当硬件产生空闲中断时
 *
 * @param  UART_enum 串口号
 * @return 数据流是否中止
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
 * @brief 发送数据
 * @details 向发送缓冲区写入数据并启动发送
 *
 * @param  UART_enum 串口号
 * @param  buffer 写入的数据指针
 * @param  size  写入的数据字节数
 * @return 已经写入的数据数
 *   @retval 无
 */
uint16_t uart_WriteData(UART_enum num, const void *buffer, uint16_t size)
{
    stm32_serial_stk *ch;
    uint8_t *ptr;

    ch = &serial_dev[num];
    ptr = (uint8_t *)buffer;

    /// 空闲字段长度
    USART_ITConfig(ch->dev, USART_IT_TXE, DISABLE);	///< 禁止发送中断,确保下面的代码与中断互斥
    while (size--)
    {
        ring_buffer_save(&ch->tx_buff, *(ptr++));
    }
    USART_ITConfig(ch->dev, USART_IT_TXE, ENABLE); 	///< 使能发送中断
    
    return (ptr - (uint8_t *)buffer);
}

/**
 * @brief 串口初始化
 * @details 串口初始化入口函数
 *
 * @param 无
 * @return 无
 *   @retval 无
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
