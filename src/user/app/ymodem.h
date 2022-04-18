#ifndef __YMODEM_H__
#define __YMODEM_H__
/*******************************************************************************************************/
#include "type.h"

#define YMODEM_BUFF_LEN             (3 + 1024 + 2 + 1)

/* 数据包大小. */
#define YMODEM_PACKET_128_SIZE      (128)
#define YMODEM_PACKET_1024_SIZE     (1024)
#define YMODEM_PACKET_CRC_SIZE      (2)

/* 协议定义的字节. */
#define YMODEM_SOH                  (1)     /**< 包头 (128 bytes). */
#define YMODEM_STX                  (2)     /**< 包头 (1024 bytes). */
#define YMODEM_EOT                  (4)     /**< 传输结束. */
#define YMODEM_ACK                  (6)     /**< 应答. */
#define YMODEM_NAK                  (0x15)  /**< 非应答. */
#define YMODEM_CAN                  (0x18)  /**< 取消. */
#define YMODEM_C                    (0x43)  /**< ASCII“C”，要通知上位机，我们要用CRC16. */

/*******************************************************************************************************/
//接收缓冲区
typedef struct
{
  uint8   buff[YMODEM_BUFF_LEN];
  uint16  num;					  //数据计数器
  uint16  len;				   	//数据包长度
  uint8	  tmout_cunt;
  uint8	  tmout_t;
} ymodem_buff_stk;

//数据帧格式定义
typedef union
{
 uint8 array[YMODEM_BUFF_LEN];
 struct
 { 
    uint8 h1;
    uint8 sn;
    uint8 sn_c;
    uint8 dat[1];
    uint8 crc_h;
    uint8 crc_l;
 }stk;
}ymodem_frame_stk;

//传输上下文
typedef struct
{
   uint8  task_ps;
   uint8  comm_tmout; 
   uint8  eot_num;
   uint16 pkgs_num;
   int    flash_partition;
   uint32 file_size;
   uint32 file_offset;
}ymodem_context_stk;

/*******************************************************************************************************/
void ymodem_start(void *arg);
void ymodem_task(void);

#endif
/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
