#ifndef __UART_H__
#define __UART_H__
/********************************************************************************************************/
#define  UART_NUM           1           //1个串口

#define  UART1_TX_BUFFLEN	1024        //调试串口
#define  UART1_RX_BUFFLEN	512

/********************************************************************************************************/
#include "stm32f10x.h"
#include "type.h"
#include "ringBuffer.h"

typedef struct
{
    USART_TypeDef *dev;
    ring_buffer rx_buff;
    ring_buffer tx_buff;
    bool rx_eof;
    bool tx_busy;
} stm32_serial_stk;

typedef enum
{
    UART1_em = 0,
} UART_enum;
/********************************************************************************************************/
void uart_Init(void);
void uart_DeInit(void);
void uart_band_set(UART_enum num, int band);
void uart_buffer_clear(UART_enum num);
int  uart_GetRcvDataEOF(UART_enum num);
uint16_t uart_GetRcvDataNum(UART_enum num);
uint16_t uart_ReadData(UART_enum num, void *buffer, uint16_t size);
uint16_t uart_WriteData(UART_enum num, const void *buffer, uint16_t size);

/********************************************************************************************************/
#endif
