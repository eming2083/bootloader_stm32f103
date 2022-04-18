#include "stm32f10x.h"
#include "type.h"
#include "ringBuffer.h"

/**********************************************************************************
** ��������:ring_buffer_init()
** ��������:��ʼ��
** ��������:��
** ����ʱ��:2009-4-23
** ��һ���޸�ʱ��:��
***********************************************************************************/
void ring_buffer_init(ring_buffer *ring, uint8 *buff, uint16 size)
{
    ring->buffer = buff;
    ring->buffer_size = size;
    ring->read_index = 0;
    ring->save_index = 0;
}

/**********************************************************************************
** ��������:ring_buffer_save
** ��������:����һ���ֽ�
** ��������:��
** ����ʱ��:2009-4-23
** ��һ���޸�ʱ��:��
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
** ��������:ring_buffer_read
** ��������:��ȡ����
** ��������:��
** ����ʱ��:2009-4-23
** ��һ���޸�ʱ��:��
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
** ��������:ring_buffer_save
** ��������:����һ���ֽ�
** ��������:��
** ����ʱ��:2009-4-23
** ��һ���޸�ʱ��:��
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
** ��������:ring_buffer_read_isr
** ��������:��ȡ����
** ��������:��
** ����ʱ��:2009-4-23
** ��һ���޸�ʱ��:��
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
** ��������:ring_buffer_data_num()
** ��������:��ȡ��ǰ��������������
** ��������:��
** ����ʱ��:2009-4-23
** ��һ���޸�ʱ��:��
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
** ��������:ring_buffer_isEmpty()
** ��������:�ж϶����Ƿ��
** ��������:��
** ����ʱ��:2009-4-23
** ��һ���޸�ʱ��:��
***********************************************************************************/
int ring_buffer_isEmpty(ring_buffer *ring)
{
    if(ring->save_index == ring->read_index) //�п�
        return TRUE;
    else
        return FALSE;
}

