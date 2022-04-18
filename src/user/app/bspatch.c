/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <limits.h>
#include <ustdio.h>
#include <stdlib.h>
#include <string.h>
#include "bspatch.h"

#define WRITE_BLOCK_SIZE    1024

int64_t offtin(uint8_t *buf)
{
    int64_t y;

    y = buf[7]&0x7F;
    y = y * 256;
    y += buf[6];
    y = y * 256;
    y += buf[5];
    y = y * 256;
    y += buf[4];
    y = y * 256;
    y += buf[3];
    y = y * 256;
    y += buf[2];
    y = y * 256;
    y += buf[1];
    y = y * 256;
    y += buf[0];

    if (buf[7]&0x80) y = -y;

    return y;
}

int bspatch(const uint8_t* old, int64_t oldsize, int64_t newsize, struct bspatch_stream* stream)
{
    uint8_t buf[8];
    uint8_t *buf_data;
    int64_t oldpos, newpos, len;
    int64_t ctrl[3];
    int64_t i;

    buf_data = (uint8_t*)malloc(WRITE_BLOCK_SIZE + 1);
    if(buf_data == NULL)return -1;
    
    oldpos = 0;
    newpos = 0;
    while (newpos < newsize)
    {
        /* Read control data */
        for (i = 0; i <= 2; i++)
        {
            if (stream->read(stream, buf, 8))
                return -1;
            ctrl[i] = offtin(buf);
        };

        /* Sanity-check */
        if (ctrl[0] < 0 || ctrl[0] > INT_MAX ||
                ctrl[1] < 0 || ctrl[1] > INT_MAX ||
                newpos + ctrl[0] > newsize)
            return -1;

        /* Read diff string */
        while(ctrl[0] > 0)
        {
            if(ctrl[0] > WRITE_BLOCK_SIZE)len = WRITE_BLOCK_SIZE;
            else len = ctrl[0];
            if (stream->read(stream, buf_data, len))return -1;
            for (i = 0; i < len; i++)
            {
                if ((oldpos + i >= 0) && (oldpos + i < oldsize))
                {
                    buf_data[i] += old[oldpos + i];
                }
            }
            stream->write(stream, buf_data, len);
            ctrl[0] -= len;
            oldpos += len;
            newpos += len;
        }

        /* Sanity-check */
        if (newpos + ctrl[1] > newsize)
            return -1;

        /* Read extra string */
        while(ctrl[1] > 0)
        {
            if(ctrl[1] > WRITE_BLOCK_SIZE)len = WRITE_BLOCK_SIZE;
            else len = ctrl[1];
            if (stream->read(stream, buf_data, len))return -1;
            stream->write(stream, buf_data, len);
            ctrl[1] -= len;
            newpos += len;
        }

        /* Adjust pointers */
        oldpos += ctrl[2];
        //printk("newpos = %d, ctrl[0] = %ld, ctrl[1] = %ld, ctrl[2] = %ld\n", newpos, ctrl[0], ctrl[1], ctrl[2]);
    };

    if(buf_data !=NULL)free(buf_data);
    
    return 0;
}

//===========================================================
#include "lzma_decompress.h"
#include "type.h"
#include "flash.h"
#include "vFile.h"

#define DCOMPRESS_BUFFER_SIZE   1024
static uint8_t *diff_data_buff;
static int diff_data_len, diff_data_fp;
static int new_data_fp;

static int patch_data_read(const struct bspatch_stream* stream, void* buffer, int length)
{
    uint8_t *dp = (uint8_t*)buffer;
    vFile* pf;

    pf = (vFile*) stream->opaque_r;

    for(int i = 0; i < length; i++)
    {
        if(diff_data_len == 0)
        {
            diff_data_len = lzma_decompress_read(pf, diff_data_buff, DCOMPRESS_BUFFER_SIZE);
            if(diff_data_len > 0)
            {
                diff_data_fp = 0;
            }
            else
            {
                return -1;
            }
        }
        if(diff_data_len > 0)
        {
            *(dp++) = diff_data_buff[diff_data_fp++];
            diff_data_len--;
        }
    }

    return 0;
}

static int new_data_write(const struct bspatch_stream* stream, void* buffer, int length)
{
    uint32_t file_addr;

    file_addr = *((uint32_t*)stream->opaque_w);
    
    //flash_write_without_erase������Ϊ�Ƕ�����ʶ�����
    if(FALSE == flash_write_without_erase(flash_bak, file_addr + new_data_fp, buffer, length))
    {
        //�������֧�ַǶ���д��,�����ٶ���
        flash_write(flash_bak, file_addr + new_data_fp, buffer, length);
    }
    new_data_fp += length;
    printk(".");
    //printk("flash write %d\r\n", new_data_fp);
    
    return(TRUE);
}

static void patch_data_read_finish(void)
{
    lzma_decompress_finish();

}

int iap_patch(const uint8_t* old, uint32_t oldsize, const uint8_t* patch, uint32_t patchsize, uint32_t newfile)
{
    vFile *patch_fp;
    struct bspatch_stream stream;
    uint8_t header[24];
    int64_t newsize;

    //��ʼ��ȫ�ֱ���
    diff_data_len = 0;
    diff_data_fp = 0;
    new_data_fp = 0;
    patch_fp = vfopen(patch, patchsize);
    
    //��ȡ����ļ�ͷ
    vfread(patch_fp, header, sizeof(header));
    if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0)return(0);
    //�����¹̼�����
    newsize = offtin(header + 16);
    if(newsize < 0)return(0);
    
    //�����ڴ�
    diff_data_buff = malloc(DCOMPRESS_BUFFER_SIZE);
    if(diff_data_buff == NULL)return(0);
    
    //׼���ϲ��ļ�
    stream.read = patch_data_read;
    stream.opaque_r = (void*)patch_fp;
    stream.write = new_data_write;
    stream.opaque_w = &newfile;
    
    bspatch(old, oldsize, newsize, &stream);
    
    //�ͷ��ڴ�
    patch_data_read_finish();
    free(diff_data_buff);
    vfclose(patch_fp);
    
    return((int)newsize);
}








