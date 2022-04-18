/**
  ******************************************************************************
  * @file           ustdio.h
  * @author         古么宁
  * @brief          非标准化打印输出头文件
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
*/

#ifndef _unstandard_format_
#define _unstandard_format_

/* Public macro ------------------------------------------------------------*/

#define printl(ptr,len)  do{if (current_puts) current_puts(ptr,len);}while(0)

#define color_printk(color,...) \
	do{\
		printk((char *)color);\
		printk(__VA_ARGS__);  \
		printk(default_color);\
	}while(0)

#define Warnings(...) \
	do{\
		printk("%sWarning:",(char *)red);\
		printk(__VA_ARGS__);  \
		printk(default_color);\
	}while(0)
	
	
#define Errors(...)   \
	do{\
		printk("%sERROR:",(char *)light_red);\
		printk(__VA_ARGS__);  \
		printk(default_color);\
	}while(0)

		
#define Debug_Here() printk("%sHere is %s()-%d\r\n%s",(char *)green,__FUNCTION__,__LINE__,default_color)

#define Error_Here() printk("%sError on %s()-%d\r\n%s",(char *)light_red,__FUNCTION__,__LINE__,default_color)


/* Public types ------------------------------------------------------------*/

typedef void (*fmt_puts_t)(const char * strbuf,unsigned short len);//

/* Public variables ---------------------------------------------------------*/

extern  fmt_puts_t current_puts;
extern  fmt_puts_t default_puts;

extern const  char	none        [];  
extern const  char	black       [];  
extern const  char	dark_gray   [];  
extern const  char	blue        [];  
extern const  char	light_blue  [];  
extern const  char	green       [];  
extern const  char	light_green [];  
extern const  char	cyan        [];  
extern const  char	light_cyan  [];  
extern const  char	red         [];  
extern const  char	light_red   [];  
extern const  char	purple      [];  
extern const  char	light_purple[];  
extern const  char	brown       [];  
extern const  char	yellow      [];  
extern const  char	light_gray  [];  
extern const  char	white       []; 
extern char  * default_color;




/* Public function prototypes -----------------------------------------------*/
#include "usr_cfg.h"

#if CONFIG_SHELL_ENABLE
void    printk(const char* fmt, ...);
#else
#define printk(fmt_puts_t, ...)
#endif

int sprintk(char * buffer ,const char * fmt , ...);

#endif

