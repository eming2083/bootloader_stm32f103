#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__
/*******************************************************************************************************/
#include "type.h"

typedef struct
{
    uint8   *buffer;
    uint16  buffer_size;
    uint16  read_index;
    uint16  save_index;
} ring_buffer;

/*******************************************************************************************************/
void ring_buffer_init(ring_buffer *ring, uint8 *buff, uint16 size);

void ring_buffer_save(ring_buffer *ring, uint8 dat);
int ring_buffer_read(ring_buffer *ring, uint8 *buffer, uint16 size);

void ring_buffer_save_isr(ring_buffer *ring, uint8 dat);
int ring_buffer_read_isr(ring_buffer *ring, uint8 *buffer, uint16 size);

uint16 ring_buffer_data_num(ring_buffer *ring);
int ring_buffer_isEmpty(ring_buffer *ring);

#endif
/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/
