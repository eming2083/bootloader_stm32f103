#ifndef __TYPE_H__
#define __TYPE_H__
/********************************************************************************************************/
#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef ERROR
#define ERROR 0xFF
#endif

typedef unsigned char       bool;
typedef unsigned char       uint8;	    /* defined for unsigned 8-bits integer variable 	�޷���8λ���ͱ���  */
typedef signed   char       int8;       /* defined for signed 8-bits integer variable		�з���8λ���ͱ���  */
typedef unsigned short      uint16;	    /* defined for unsigned 16-bits integer variable 	�޷���16λ���ͱ��� */
typedef signed   short      int16; 	    /* defined for signed 16-bits integer variable 		�з���16λ���ͱ��� */
typedef unsigned long       uint32;     /* defined for unsigned 32-bits integer variable 	�޷���32λ���ͱ��� */
typedef signed   long       int32; 	    /* defined for signed 32-bits integer variable 		�з���32λ���ͱ��� */
typedef float               fp32;  	    /* single precision floating point variable (32bits)�����ȸ�������32λ���ȣ� */
typedef unsigned long long  uint64;     /* defined for unsigned 64-bits integer variable 	�޷���64λ���ͱ��� */
typedef signed   long long  int64; 	    /* defined for signed 64-bits integer variable 		�з���64λ���ͱ��� */
typedef double              fp64;       /* double precision floating point variable (64bits)˫���ȸ�������64λ���ȣ� */
/********************************************************************************************************/
#define BigtoLittle16(x)    ((((uint16)(x) & 0xff00) >> 8) | (((uint16)(x) & 0x00ff) << 8))
#define BigtoLittle32(x)    ((((uint32)(x) & 0xff000000) >> 24) | \
                            (((uint32)(x) & 0x00ff0000) >> 8) | \
                            (((uint32)(x) & 0x0000ff00) << 8) | \
                            (((uint32)(x) & 0x000000ff) << 24))
/********************************************************************************************************/
//����һ�����ϣ���һ��64λ���ֲ��4��8λ�����Ա��ڼ���
//M3�ں�ΪС�˷�ʽ,�����ֽ��ڵ�λ,���ֽ��ڸ�λ
typedef union
{
    uint64 d64;
    uint8  d8[8];
} U64_U8;
//����һ�����ϣ���һ��64λ���ֲ��4��8λ�����Ա��ڼ���
//M3�ں�ΪС�˷�ʽ,�����ֽ��ڵ�λ,���ֽ��ڸ�λ
typedef union
{
    fp64   f64;
    uint8  d8[8];
} F64_U8;
//����һ�����ϣ���һ��32λ���ֲ��4��8λ�����Ա��ڼ���
//M3�ں�ΪС�˷�ʽ,�����ֽ��ڵ�λ,���ֽ��ڸ�λ
typedef union
{
    uint32 d32;
    uint8  d8[4];
} U32_U8;
/********************************************************************************************************/
//����һ�����ϣ���һ��16λ���ֲ��2��8λ�����Ա��ڼ���
typedef union
{
    uint16 d16;
    uint8  d8[2];
} U16_U8;
/********************************************************************************************************/
//����һ�����ϣ���һ��16λ���ֲ��2��8λ�����Ա��ڼ���
typedef union
{
    int16  d16;
    uint8  d8[2];
} I16_U8;
/********************************************************************************************************/
#endif
