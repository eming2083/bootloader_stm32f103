#include "stm32f10x.h"
#include "type.h"
#include "ringBuffer.h"

/**********************************************************************************
** 函数名称:ring_buffer_init()
** 函数功能:初始化
** 函数参数:无
** 创建时间:2009-4-23
** 第一次修改时间:无
***********************************************************************************/
void ring_buffer_init(ring_buffer *ring, uint8 *buff, uint16 size)
{
    ring->buffer = buff;
    ring->buffer_size = size;
    ring->read_index = 0;
    ring->save_index = 0;
}

/**********************************************************************************
** 函数名称:ring_buffer_save
** 函数功能:保存一个字节
** 函数参数:无
** 创建时间:2009-4-23
** 第一次修改时间:无
***********************************************************************************/
void ring_buffer_save(ring_buffer *ring, uint8 dat)
{
    /* save character */
    ring->buffer[ring->save_index] = dat;
    ring->save_index++;
    
    if(ring->save_index >= ring->buffer_size)
        ring->save_index = 0;

    /* if the next position is read index, discard this 'read char' */
    if (ring->save_index == ring->read_index)
    {
        ring->read_index ++;
        if(ring->read_index >= ring->buffer_size)
            ring->read_index = 0;
    }
}

/**********************************************************************************
** 函数名称:ring_buffer_read
** 函数功能:读取数据
** 函数参数:无
** 创建时间:2009-4-23
** 第一次修改时间:无
***********************************************************************************/
int ring_buffer_read(ring_buffer *ring, uint8 *buffer, uint16 size)
{
    uint8 *ptr;
    
    ptr = buffer;
    while (size--)
    {
        if (ring->read_index != ring->save_index)
        {
            /* read a character */
            *(ptr++) = ring->buffer[ring->read_index];

            /* move to next position */
            ring->read_index++;
            if (ring->read_index >= ring->buffer_size)
                ring->read_index = 0;
        }
        else
        {
            break;
        }
    }
 
    return (ptr - buffer);
}

/**********************************************************************************
** 函数名称:ring_buffer_save
** 函数功能:保存一个字节
** 函数参数:无
** 创建时间:2009-4-23
** 第一次修改时间:无
***********************************************************************************/
void ring_buffer_save_isr(ring_buffer *ring, uint8 dat)
{
    /* save character */
    ring->buffer[ring->save_index] = dat;
    ring->save_index++;
    
    if(ring->save_index >= ring->buffer_size)
        ring->save_index = 0;

    /* if the next position is read index, discard this 'read char' */
    if (ring->save_index == ring->read_index)
    {
        ring->read_index ++;
        if(ring->read_index >= ring->buffer_size)
            ring->read_index = 0;
    }
}

/**********************************************************************************
** 函数名称:ring_buffer_read_isr
** 函数功能:读取数据
** 函数参数:无
** 创建时间:2009-4-23
** 第一次修改时间:无
***********************************************************************************/
int ring_buffer_read_isr(ring_buffer *ring, uint8 *buffer, uint16 size)
{
    uint8 *ptr;
    
    ptr = buffer;
    while (size--)
    {
        if (ring->read_index != ring->save_index)
        {
            /* read a character */
            *(ptr++) = ring->buffer[ring->read_index];

            /* move to next position */
            ring->read_index++;
            if (ring->read_index >= ring->buffer_size)
                ring->read_index = 0;
        }
        else
        {
            break;
        }
    }
 
    return (ptr - buffer);
}

/**********************************************************************************
** 函数名称:ring_buffer_data_num()
** 函数功能:获取当前缓冲区数据数量
** 函数参数:无
** 创建时间:2009-4-23
** 第一次修改时间:无
***********************************************************************************/
uint16 ring_buffer_data_num(ring_buffer *ring)
{
    if(ring->save_index >= ring->read_index)
    {
        return(ring->save_index - ring->read_index);
    }
    else
    {
        return(ring->save_index + ring->buffer_size - ring->read_index);
    }
}

/**********************************************************************************
** 函数名称:ring_buffer_isEmpty()
** 函数功能:判断队列是否空
** 函数参数:无
** 创建时间:2009-4-23
** 第一次修改时间:无
***********************************************************************************/
int ring_buffer_isEmpty(ring_buffer *ring)
{
    if(ring->save_index == ring->read_index) //判空
        return TRUE;
    else
        return FALSE;
}

