/**
  ******************************************************************************
  * @file    mylib.c
  * @author  eming
  * @version V1.0.0
  * @date    2022-03-21
  * @brief   ����һЩ���ú���
  ******************************************************************************
  */
#include "stdint.h"
#include "ustdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "type.h"

/**
 * @brief ��У��
 * @details �ֽ�����ĺ�У�麯��
 *
 * @param *p ����ָ��
 * @param size ���鳤��
 * @return ��ͽ��
 *   @retval ��
 */
uint32_t mylib_sum(const uint8_t *p, uint16_t size)
{
    uint32_t sum = 0;

    while(size--)
    {
        sum += *(p++);
    }
    return(sum);
}

/**
 * @brief ���ݱȽ�
 * @details �ֽ���������ݱȽ�
 *
 * @param *p1 ����1ָ��
 * @param *p2 ����2ָ��
 * @param size �Ƚϵ����ݿ��С
 * @return �ȽϽ��
 *   @retval TRUE ���
 *   @retval FALSE �����
 */
int mylib_memcmp(const void *p1, const void *p2, int size)
{
    uint8_t *bp1, *bp2;
    
    bp1 = (uint8_t*)p1;
    bp2 = (uint8_t*)p2;
    while(size--)
    {
        if(*(bp1++) != *(bp2++))return(FALSE);
    }
    return(TRUE);
}

/**
 * @brief ���ݱȽ�
 * @details �ֽ�������ָ�����ݱȽ�
 *
 * @param *p1 ����ָ��
 * @param value ���Ƚϵ���ֵ
 * @param size �Ƚϵ����ݿ��С
 * @return �ȽϽ��
 *   @retval TRUE ���
 *   @retval FALSE �����
 */
int mylib_memcmp_b(uint8_t *p1, uint8_t value, int size)
{
    while(size--)
    {
        if(*(p1++) != value)return(FALSE);
    }
    return(TRUE);
}

/**
 * @brief ��ʱ����
 * @details ���Ϊ1uS����ʱ����
 *
 * @param us ��Ҫ��ʱ��΢����
 * @return ��
 *   @retval ��
 */
void mylib_uDelay(uint32_t us)
{
    int i;
    
    for(; us > 0; us--)
    {
        for(i = 0; i < 40; i++);
    }
}

/**
 * @brief CRC16����
 * @details �����ֽ������CRC16
 *
 * @param *p ����ָ��
 * @param size ���鳤��
 * @return ������
 *   @retval uint16_t CRC16ֵ
 */
uint16_t mylib_crc16(uint8_t *p, uint16_t size)
{
    uint16_t crc;                        //У����
    uint16_t i;
    uint16_t j;
    crc = 0xFFFF;
    for (i = 0; i < size; i++)
    {
        crc ^= *p;
        for (j = 0; j < 8; j++)
        {
            if ((crc & 1) == 1)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
        p++;
    }
    return(crc);
}

/**
 * @brief 16λ��С��ת��
 * @details ���ݴ��С�λ���ת������
 *
 * @param *frm Դ����ָ��
 * @param *to Ŀ������ָ��
 * @param size ת�������ݸ���(����)
 * @return ��
 *   @retval ��
 */
void mylib_BigtoLittle16(const void *frm, void *to, uint16_t size)
{
    uint8_t  buff[2];
    uint8_t 	*fp, *tp;

    fp = (uint8_t *)frm;
    tp = (uint8_t *)to;
    while(size--)
    {
        buff[0] = fp[0];
        buff[1] = fp[1];
        tp[0] 	= buff[1];
        tp[1] 	= buff[0];

        tp  += 2;
        fp += 2;
    }
}

/**
 * @brief 32λ��С��ת��
 * @details ���ݴ��С�λ���ת������
 *
 * @param *frm Դ����ָ��
 * @param *to Ŀ������ָ��
 * @param size ת�������ݸ���(��)
 * @return ��
 *   @retval ��
 */
void mylib_BigtoLittle32(const void *frm, void *to, uint16_t size)
{
    uint8_t buff[4];
    uint8_t *fp, *tp;

    fp = (uint8_t *)frm;
    tp = (uint8_t *)to;
    while(size--)
    {
        buff[0] = fp[0];
        buff[1] = fp[1];
        buff[2] = fp[2];
        buff[3] = fp[3];
        tp[0] 	= buff[3];
        tp[1] 	= buff[2];
        tp[2] 	= buff[1];
        tp[3] 	= buff[0];

        tp  += 4;
        fp += 4;
    }
}

/**
 * @brief 64λ��������С��ת��
 * @details ���ݴ��С�λ���ת������
 *
 * @param dat ������
 * @return ת�����
 *   @retval ת����ĸ�����
 */
double mylib_BigtoLittle_fp64(double dat)
{
    F64_U8 tmp1, tmp2;

    tmp1.f64 = dat;
    tmp2.d8[0] = tmp1.d8[7];
    tmp2.d8[1] = tmp1.d8[6];
    tmp2.d8[2] = tmp1.d8[5];
    tmp2.d8[3] = tmp1.d8[4];
    tmp2.d8[4] = tmp1.d8[3];
    tmp2.d8[5] = tmp1.d8[2];
    tmp2.d8[6] = tmp1.d8[1];
    tmp2.d8[7] = tmp1.d8[0];
    
    return(tmp2.f64);
}

/**
 * @brief BCD���뺯��
 * @details ����������תBCD��
 *
 * @param hex ����������
 * @return ת�����
 *   @retval BCD��
 */
uint8_t mylib_HEXtoBCD(uint8_t hex)
{
    uint8_t bcd = 0;

    bcd = hex / 10;
    bcd <<= 4;
    bcd += hex % 10;

    return(bcd);
}

/**
 * @brief BCD���뺯��
 * @details BCD��ת����������
 *
 * @param bcd BCD������
 * @return ת�����
 *   @retval ����������
 */
uint8_t mylib_BCDtoHEX(uint8_t bcd)
{
    uint8_t hex = 0;

    hex = ((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F);
    return(hex);
}

/**
 * @brief ��ֵת�ַ���
 * @details ������ֵת�ַ���
 *
 * @param value ������ֵ
 * @param *str ת������ַ���ָ��
 * @param radix ����
 * @return ������ַ���ָ��
 */
char *itoa(int value, char *str, uint8_t radix)
{
    static char szMap[] =
    {
        '0', '1', '2', '3', '4', '5',
        '6', '7', '8', '9', 'a', 'b',
        'c', 'd', 'e', 'f', 'g', 'h',
        'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z'
    };  // �ַ�ӳ���
    int nCount = -1, nIndex;
    char *pStr = str, nTemp;
    unsigned int nValue = *(unsigned *)&value;

    if(radix >= 2 && radix <= 36 )          //����radix������2��36֮��
    {
        if(value < 0 && radix == 10)        //����Ǹ���������λ��Ӹ��ţ������ַ���ǰ��
        {
            *pStr++ = '-';
            value = -value;                 //תΪ������
        }
        do                                  //ѭ��ת��ÿһ�����֣�ֱ������
        {
            pStr[++nCount] = szMap[nValue % radix];
            nValue /= radix;
        }
        while(nValue > 0);                  //ת���������ַ����Ƿ���
        nIndex = (nCount + 1) / 2;          //�����һ��ĳ���
        while(nIndex-- > 0)                 //���ַ������ַ���ת
        {
            nTemp = pStr[nIndex];
            pStr[nIndex] = pStr[nCount - nIndex];
            pStr[nCount - nIndex] = nTemp;
        }
    }
    pStr[nCount + 1] = '\0';                // �ý�����
    return str;
}

/**
 * @brief ����ת�ַ���
 * @details �ֽ�����ת�ַ���
 *
 * @param *str ת������ַ���
 * @param bytes ��ת�����ֽ�����
 * @param size ��ת�����ֽ����鳤��
 * @return ת�������ַ�������
 */
int mylib_bytes_to_string(char *str, const uint8_t *bytes, int size)
{
    int i;

    char hexArray[] = "0123456789ABCDEF";

    for (i = 0; i < size; i++)
    {
        int v = bytes[i] & 0xFF;
        str[i * 2] = hexArray[v >> 4];
        str[i * 2 + 1] = hexArray[v & 0x0F];
    }
    str[size * 2] = '\0';

    return (size * 2);
}

/**
 * @brief �ַ���ת����
 * @details �ַ���ת�ֽ�����
 *
 * @param *str ��ת�����ַ���
 * @param bytes ת�������ֽ�����
 * @return ת�������ֽ����鳤��
 */
int mylib_string_to_bytes(char *str, uint8_t *bytes)
{
    uint8_t 	tmp, tmp1;
    uint16_t len;
    uint16_t dat_len;

    len = strlen(str);
    if(len % 2)				  			//����Ƿ�Ϊż��
    {
        str[len] = '0';					//��0
        len++;
        str[len] = '\0';
    }
    dat_len = len / 2;
    while(len)
    {
        if((*str > 47) && (*str < 58))	  	//0~9
        {
            tmp = *str - 48;
        }
        else if((*str > 64) && (*str < 71))	//A~F
        {
            tmp = *str - 55;
        }
        else if((*str > 96) && (*str < 103))	//a~f
        {
            tmp = *str - 87;
        }
        else
        {
            tmp = 32;
        }
        tmp <<= 4;
        str++;
        if((*str > 47) && (*str < 58))	  	//0~9
        {
            tmp1 = *str - 48;
        }
        else if((*str > 64) && (*str < 71))	//A~F
        {
            tmp1 = *str - 55;
        }
        else if((*str > 96) && (*str < 103))	//a~f
        {
            tmp1 = *str - 87;
        }
        else
        {
            tmp1 = 32;
        }
        tmp += tmp1;
        *(bytes++) = tmp;
        str++;
        len -= 2;
    }
    
    return(dat_len);
}

/**
 * @brief ��Сдת��
 * @details ���ַ����еĴ�д�ַ���ΪСд�ַ�
 *
 * @param *str ��ת�����ַ���
 * @return ת�������ַ���
 */
char *strlwr(char *str)
{
    uint16_t len;
    char   *p;

    len = strlen(str);
    p = str;
    while(len--)
    {
        if((*p > 64) && (*p < 91))
        {
            *p += 32;
        }
        p++;
    }
    return(str);
}

/**
 * @brief �ַ�����ȡ����
 * @details �ַ�����ȡ�����ַ�,ȥ����������
 *
 * @param *str ��������ַ���
 * @param *num ����ַ���
 * @return ��ȡ�����ַ�������
 */
int strval(char *str, char *num)
{
    int len = 0;

    while(*str != '\0')
    {
        if((*str > 47) && (*str < 58))
        {
            *num = *str;
            num++;
            len++;
        }
        str++;
    }
    *num = '\0';
    
    return(len);
}

/**
 * @brief ��ӡ16��������
 * @details ���ڵ���ʱ�������������
 *
 * @param *p ����������ָ��
 * @param size ���ݳ���
 * @return ��
 */
void printk_hex(const uint8_t *p, int size)
{
    int i;

    for(i = 0; i < size; i++)
    {
        printk(" %02X", *(p++));
    }
}

/**
 * @brief utc��תʱ��
 * @details ����ʱ������
 *
 * @param utc_s utc��
 * @param *t ת�����
 * @param tz ʱ��
 * @return ��
 */
void utc_to_time(uint32_t utc_s, struct tm *t, int tz)
{
    #define SECOND_OF_DAY     86400      		    //1�������    
    uint16_t i, j, iDay;
    uint32_t lDay;
    const uint8_t DayOfMon[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    utc_s += 3600 * tz;    //ʱ��

    lDay = utc_s / SECOND_OF_DAY;
    utc_s = utc_s % SECOND_OF_DAY;

    i = 1970;										//����ȡֵΪ0-6
    t->tm_wday = (lDay + 3) % 7;					//1970��1��1��������4
    while(lDay > 365)
    {
        if(((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0))    	// ����
            lDay -= 366;
        else
            lDay -= 365;
        i++;
    }
    if((lDay == 365) && !(((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0))) //ƽ��
    {
        lDay -= 365;
        i++;
    }
    t->tm_year = i - 1900;     					//�õ����
    for(j = 0; j < 12; j++)     		        //�����·�
    {
        if((j == 1) && (((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0)))
            iDay = 29;
        else
            iDay = DayOfMon[j];
        if(lDay >= iDay) lDay -= iDay;
        else break;
    }
    t->tm_mon  = j;
    t->tm_mday = lDay + 1;
    t->tm_hour = utc_s / 3600;
    t->tm_min  = (utc_s % 3600) / 60;
    t->tm_sec  = (utc_s % 3600) % 60;
}

/**
 * @brief ����������һ���е���һ��
 * @details �������ڵ�������,2�µ���29����
 *
 * @param utc_s utc��
 * @param *t ת�����
 * @param tz ʱ��
 * @return ��
 */
int mylib_day_index(uint8_t mon, uint8_t day)
{
    const  uint8_t  Line_Mon_Len[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int index = 0;
    int  i;

    if((mon < 1) || (mon > 12))return(0); //�����ж�
    if((day < 1) || (day > 31))return(0);

    for(i = 1; i < mon; i++)
    {
        index += Line_Mon_Len[i - 1];
    }
    index += day - 1;

    return(index);
}

/**
 * @brief 16�����ַ���ת����
 * @details 16�����ַ���ת����,���԰���"0x"ͷ
 *
 * @param *s �ַ���
 * @return ��ȡ����ֵ
 */
int tolower(int c)  
{  
    if (c >= 'A' && c <= 'Z')  
    {  
        return c + 'a' - 'A';  
    }  
    else  
    {  
        return c;  
    }  
}

uint32_t htoi(const char *s)  
{  
    int i;  
    uint32_t n = 0; 

    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))  
    {  
        i = 2;  
    }  
    else  
    {  
        i = 0;  
    }  
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)  
    {  
        if (tolower(s[i]) > '9')  
        {  
            n = 16 * n + (10 + tolower(s[i]) - 'a');  
        }  
        else  
        {  
            n = 16 * n + (tolower(s[i]) - '0');  
        }  
    }  
    return n;  
} 

/**
 * @brief ���ض��ڴ�Ĵ�С
 * @details ���ض��ڴ�Ĵ�С
 *
 * @param *mp ���ڴ�ָ��
 * @return ���ڴ�Ĵ�С
 */
uint32_t _msize(const void *mp)
{
    return(*((int*)((uint32_t)mp - 4)));
}

/*******************************************************************************************************
**                            End Of File
********************************************************************************************************/


