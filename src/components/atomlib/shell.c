/**
  ******************************************************************************
  * @file           shell.c
  * @author         古么宁
  * @brief          shell 命令解释器，支持  TAB 键命令补全，上下左右箭头 ，BACKSPACE回删
  * @note
  * <pre>
  * 使用步骤:
  *    0.初始化硬件部分。
  *    1.编写硬件对应的void puts(char * buf , uint16_t len) 发送函数。
  *    2.shell_init(sign,puts) 初始化输入标志和默认输出。
  *    3.新建一个  shellinput_t shellx , 初始化输出 shell_input_init(&shellx,puts,...);
  *    4.接收到一包数据后，调用 shell_input(shellx,buf,len)
  *    *.  需要注册命令则调用宏 shell_register_command 进行注册。
  *    *.. shell_register_confirm() 可注册带选项命令([Y/N]选项)
  * </pre>
  ******************************************************************************
  *
  * COPYRIGHT(c) 2019 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "shell.h"
#include "containerof.h"
#include "usr_cfg.h"

#if CONFIG_SHELL_ENABLE
/* Private types ------------------------------------------------------------*/

union uncmd {
	struct {// 命令号分为以下五个部分  
		unsigned int CRC2      : 8;
		unsigned int CRC1      : 8;///< 低十六位为两个 crc 校验码
		unsigned int Sum       : 5;///< 命令字符的总和
		unsigned int Len       : 5;///< 命令字符的长度，5 bit ，即命令长度不能超过31个字符
		unsigned int FirstChar : 6;///< 命令字符的第一个字符
	}part;
	unsigned int ID;               ///< 由此合并为 32 位的命令码
};

/* Private macro ------------------------------------------------------------*/

#define VERSION      "V2.0.4"

#if USE_AVL_TREE 
	#define NEXT(x)          avl_next(x)
	#define FIRST(root)      avl_first(root)
	#define ROOT(root)       ((root)->avl_node)
#else 
	#define NEXT(x)          ((x)->next)
	#define FIRST(root)      ((root)->next)
	#define ROOT(root)       ((root)->next)
#endif

/* Private variables --------------------------------------------------------*/

static const  unsigned char F_CRC8_Table[256] = {//正序,高位先行 x^8+x^5+x^4+1
	0x00, 0x31, 0x62, 0x53, 0xc4, 0xf5, 0xa6, 0x97, 0xb9, 0x88, 0xdb, 0xea, 0x7d, 0x4c, 0x1f, 0x2e,
	0x43, 0x72, 0x21, 0x10, 0x87, 0xb6, 0xe5, 0xd4, 0xfa, 0xcb, 0x98, 0xa9, 0x3e, 0x0f, 0x5c, 0x6d,
	0x86, 0xb7, 0xe4, 0xd5, 0x42, 0x73, 0x20, 0x11, 0x3f, 0x0e, 0x5d, 0x6c, 0xfb, 0xca, 0x99, 0xa8,
	0xc5, 0xf4, 0xa7, 0x96, 0x01, 0x30, 0x63, 0x52, 0x7c, 0x4d, 0x1e, 0x2f, 0xb8, 0x89, 0xda, 0xeb,
	0x3d, 0x0c, 0x5f, 0x6e, 0xf9, 0xc8, 0x9b, 0xaa, 0x84, 0xb5, 0xe6, 0xd7, 0x40, 0x71, 0x22, 0x13,
	0x7e, 0x4f, 0x1c, 0x2d, 0xba, 0x8b, 0xd8, 0xe9, 0xc7, 0xf6, 0xa5, 0x94, 0x03, 0x32, 0x61, 0x50,
	0xbb, 0x8a, 0xd9, 0xe8, 0x7f, 0x4e, 0x1d, 0x2c, 0x02, 0x33, 0x60, 0x51, 0xc6, 0xf7, 0xa4, 0x95,
	0xf8, 0xc9, 0x9a, 0xab, 0x3c, 0x0d, 0x5e, 0x6f, 0x41, 0x70, 0x23, 0x12, 0x85, 0xb4, 0xe7, 0xd6,
	0x7a, 0x4b, 0x18, 0x29, 0xbe, 0x8f, 0xdc, 0xed, 0xc3, 0xf2, 0xa1, 0x90, 0x07, 0x36, 0x65, 0x54,
	0x39, 0x08, 0x5b, 0x6a, 0xfd, 0xcc, 0x9f, 0xae, 0x80, 0xb1, 0xe2, 0xd3, 0x44, 0x75, 0x26, 0x17,
	0xfc, 0xcd, 0x9e, 0xaf, 0x38, 0x09, 0x5a, 0x6b, 0x45, 0x74, 0x27, 0x16, 0x81, 0xb0, 0xe3, 0xd2,
	0xbf, 0x8e, 0xdd, 0xec, 0x7b, 0x4a, 0x19, 0x28, 0x06, 0x37, 0x64, 0x55, 0xc2, 0xf3, 0xa0, 0x91,
	0x47, 0x76, 0x25, 0x14, 0x83, 0xb2, 0xe1, 0xd0, 0xfe, 0xcf, 0x9c, 0xad, 0x3a, 0x0b, 0x58, 0x69,
	0x04, 0x35, 0x66, 0x57, 0xc0, 0xf1, 0xa2, 0x93, 0xbd, 0x8c, 0xdf, 0xee, 0x79, 0x48, 0x1b, 0x2a,
	0xc1, 0xf0, 0xa3, 0x92, 0x05, 0x34, 0x67, 0x56, 0x78, 0x49, 0x1a, 0x2b, 0xbc, 0x8d, 0xde, 0xef,
	0x82, 0xb3, 0xe0, 0xd1, 0x46, 0x77, 0x24, 0x15, 0x3b, 0x0a, 0x59, 0x68, 0xff, 0xce, 0x9d, 0xac
};

static const  unsigned char B_CRC8_Table[256] = {//反序,低位先行 x^8+x^5+x^4+1
	0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
	0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
	0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
	0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
	0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
	0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
	0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
	0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
	0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
	0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
	0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
	0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
	0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
	0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
	0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
	0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35
};

/// 索引起始点，目录根
static cmd_root_t shellcmdroot = {0};

/* Global variables ---------------------------------------------------------*/

/// 默认输出标志，可修改
char DEFAULT_INPUTSIGN[COMMANDLINE_MAX_LEN] = "~ # ";

/* Private function prototypes -----------------------------------------------*/
static void   shell_getchar     (struct shell_input * shellin , char ch);
static void   shell_backspace   (struct shell_input * shellin) ;
static void   shell_tab         (struct shell_input * shellin) ;
       void   shell_confirm     (struct shell_input * shellin ,char * info ,cmd_fn_t yestodo);

#if (COMMANDLINE_MAX_RECORD)//如果定义了历史纪录
	static char * shell_record(struct shell_input * shellin);
	static void   shell_show_history(struct shell_input * shellin,int LastOrNext);
#else
#	define shell_record(x)
#	define shell_show_history(x,y)
#endif //#if (COMMANDLINE_MAX_RECORD)//如果定义了历史纪录

/* Gorgeous Split-line ------------------------------------------------------*/

/**
  * @brief    命令匹配，根据 cmd 找到对应的控制块
  * @param    cmdindex : 命令号
  * @param    root     : 检索根，起始检索点
  * @return   返回 cmd 命令字符串对应的控制块
*/
static struct shellcommand *shell_search_cmd(cmd_root_t * root , unsigned int cmdindex)
{
	struct shellcommand * command ;
	cmd_entry_t *node = ROOT(root);

	#if USE_AVL_TREE 
		while (node) {
			command = container_of(node, struct shellcommand, node);
			if (cmdindex < command->ID)
				node = node->avl_left;
			else 
			if (cmdindex > command->ID)
				node = node->avl_right;
			else 
				return command;
		}
	#else 
		for ( ; node ; node = node->next ) {
			command = container_of(node, struct shellcommand, node);
			if (command->ID > cmdindex)
				return NULL;
			else
			if (command->ID == cmdindex)
				return command;
		}
	#endif 
  
	return NULL;
}



/**
  * @brief    新命令插入记录
  * @param    root     : 检索根，起始检索点
  * @param    newcmd   : 新命令控制块
  * @return   成功返回 0
*/
static int shell_insert_cmd(cmd_root_t * root , struct shellcommand * newcmd)
{
	struct shellcommand * command ;
	cmd_entry_t **node = &ROOT(root) ;

	#if USE_AVL_TREE 
		/* 用平衡二叉树构建查询系统 */
		cmd_entry_t *parent = NULL;

		/* Figure out where to put new node */
		while (*node) {
			command = container_of(*node, struct shellcommand, node);
			parent = *node;
			if (newcmd->ID < command->ID)
				node = &((*node)->avl_left);
			else 
			if (newcmd->ID > command->ID)
				node = &((*node)->avl_right);
			else
				return 1;
		}

		/* Add new node and rebalance tree. */
		avl_insert(root,&newcmd->node,parent,node);
	#else 
		/* 用单链表构建查询系统 */
		for ( ; *node ; node = &((*node)->next) ) {
			command = container_of(*node, struct shellcommand, node);
			if (newcmd->ID == command->ID) 
				return -1;
			else
			if (command->ID > newcmd->ID)
				break ;
		}
		newcmd->node.next = *node; 
		*node = &newcmd->node;
	#endif 
	return 0;
}


/** 
  * @brief  检出命令起始字符串为 str 的所有命令 
  * @param  str      : 起始字符串
  * @param  len      : 起始字符串长度
  * @param  checkout : 检出内存
  * @param  checkmax : 最大检出数，如果超出此数则返回 0 
  * @return 返回检出命令的条目数
*/
static int checkout(char * str,int len,struct shellcommand ** checkout , int checkmax)
{
	unsigned int index , end;
	int      matchnums = 0;
	struct shellcommand * shellcmd = NULL;
	cmd_entry_t  * node = ROOT(&shellcmdroot);

	/* 首字母相同并且长度不小于 len 的点作为起始点，下一个字母开头的点作为结束点 */
	index = ((unsigned int)(*str)<<26) | (len << 21) ;
	end = (unsigned int)(*str + 1)<<26 ; 

	/* 先找到起始匹配点 */
	#if USE_AVL_TREE 
		/* index 不存在，查找结束后的 shell_cmd 最靠近 index 用此作为起始匹配点 */
		while ( node ){
			shellcmd = container_of(node,struct shellcommand, node);	
			node = (index < shellcmd->ID) ? node->avl_left : node->avl_right;
		}

		if (shellcmd)
		   node = &shellcmd->node ;
	#else 
		/* 查找到首字母相同的点作为起始点 */
		for ( ; node ; node = NEXT(node)) { 
			shellcmd = container_of(node, struct shellcommand, node);
			if (shellcmd->ID > index)
				break;
		}
	#endif

	for( ; node ; node = NEXT(node) ) {
		/* 对比输入的字符串，如果前 len 个字符与 str 相同,把命令块记下来 */
		shellcmd = container_of(node,struct shellcommand, node);
		if (shellcmd->ID > end) {
			break ;
		}

		if (0 == memcmp(shellcmd->name, str, len)){ 
			checkout[matchnums] = shellcmd;
			if (++matchnums > 10) {
				return 0;    
			}
		}
	}

	return matchnums;
}


#if (COMMANDLINE_MAX_RECORD) //如果定义了历史纪录

/**
  * @author   古么宁
  * @brief    记录此次运行的命令及参数
  * @param    
  * @return   返回记录地址
*/
static char * shell_record(struct shell_input * shellin)
{	
	char *  history = &shellin->history[shellin->htywrt][0];
	
	shellin->htywrt  = (shellin->htywrt + 1) % COMMANDLINE_MAX_RECORD;
	shellin->htyread = shellin->htywrt;

	memcpy(history,shellin->cmdline,shellin->tail);
	history[shellin->tail] = 0;
	
	return history;
}


/**
  * @author   古么宁
  * @brief    按上下箭头键显示以往输入过的命令，此处只记录最近几次的命令
  * @param    void
  * @return   don't care
*/
static void shell_show_history(struct shell_input * shellin,int LastOrNext)
{
	int len = 0;
	
	printk("\33[2K\r%s",shellin->sign);//"\33[2K\r" 表示清除当前行

	if (!LastOrNext) //上箭头，上一条命令
		shellin->htyread = (!shellin->htyread) ? (COMMANDLINE_MAX_RECORD - 1) : (shellin->htyread - 1);
	else       //下箭头
	if (shellin->htyread != shellin->htywrt)
		shellin->htyread = (shellin->htyread + 1) % COMMANDLINE_MAX_RECORD;

	if (shellin->htyread != shellin->htywrt){ //把历史记录考到命令行内存 
		for (char * history = &shellin->history[shellin->htyread][0]; *history ; ++len)
			shellin->cmdline[len] = *history++;
	}
	
	shellin->cmdline[len] = 0; //添加结束符
	shellin->tail = len ;
	shellin->edit = len ;

	if (len)
		printl(shellin->cmdline,len); //打印命令行内容
}

#endif //#if (COMMANDLINE_MAX_RECORD) //如果定义了历史纪录



/** 
  * @brief    输入 table 键处理
  * @param    input
  * @return   don't care
*/
static void shell_tab(struct shell_input * shellin)
{
	struct shellcommand * match[10];  
	char  *  str = shellin->cmdline;
	int  len = shellin->tail;
	int matchnums = 0 ; 
	
	/* Shave off any leading spaces */
	for ( ; *str == ' ' ; ++str) {
		--len; 
	}

	if (*str == 0 || len == 0 ){
		return ;
	}

	/* 如果没有命令包含输入的字符串，返回 */
	matchnums = checkout(str,len,match,10);
	if (!matchnums){ 
		return ; 
	}

	/* 如果编辑位置不是末端，先把光标移到末端 */
	if (shellin->edit != shellin->tail) { 
		printl(&shellin->cmdline[shellin->edit],shellin->tail - shellin->edit);
		shellin->edit = shellin->tail;
	}

	if (1 == matchnums){
		/* 如果只找到了一条命令包含当前输入的字符串，直接补全命令并打印 */
		for(char * fmt = match[0]->name + len ; *fmt ;++fmt){
			shell_getchar(shellin,*fmt);
		}
		shell_getchar(shellin,' ');
	}
	else {  
		/* 如果不止一条命令包含当前输入的字符串，打印含有相同
		  字符的命令列表，并补全字符串输出直到命令区分点 */
		for(int i = 0;i < matchnums; ++i) {
			printk("\r\n\t%s",match[i]->name); 
		}

		printk("\r\n%s%s",shellin->sign,shellin->cmdline); 
		for ( ; ; ) {
			/* 把 match[] 中含有相同的字符补全到输入缓冲中 */
			for (int i = 1 ; i < matchnums ; ++i ) {
				if (match[0]->name[len] != match[i]->name[len]){
					return  ;
				}
			}
			shell_getchar(shellin,match[0]->name[len++]);
		}
	}
}


/**
  * @author   古么宁
  * @brief    如果当前打印行有输入内容，回退一个键位
  * @param    shellin : 输入交互
  * @return   don't care
*/
static void shell_backspace(struct shell_input * shellin)
{
	char   printbuf[COMMANDLINE_MAX_LEN*2]={0};//中转内存
	char * print = &printbuf[1];
	char * printend = print + (shellin->tail - shellin->edit) + 1;
	char * edit = &shellin->cmdline[shellin->edit--] ;
	char * tail = &shellin->cmdline[shellin->tail--];

	/* 当输入过左右箭头时，需要作字符串插入左移处理，并作反馈回显
	   如 abUcd 中删除U，需要左移cd，并打印两个 '\b' 使光标回到 ab 处 */
	for (char * cp = edit - 1 ; edit < tail ; *cp++ = *edit++) {
		*print++ = *edit;
		*printend++ = '\b';
	}

	printbuf[0] = '\b';
	*print = ' ';       //覆盖最后一个字符显示
	*printend++ = '\b'; //光标回显

	shellin->cmdline[shellin->tail] = 0;  //末端添加字符串结束符
	printl(printbuf,printend-printbuf);
}

/**
  * @author   古么宁
  * @brief    命令行记录输入一个字符
  * @param    shellin : 输入交互
  * @param    ascii   : 键盘输入字符
  * @return   don't care
*/
static void shell_getchar(struct shell_input * shellin , char ascii)
{
	if (shellin->tail + 1 >= COMMANDLINE_MAX_LEN){
		return ;
	}

	if (shellin->tail == shellin->edit) {
		shellin->cmdline[shellin->edit++] = ascii;
		shellin->cmdline[++shellin->tail] = 0;
		printl(&ascii,1);
	}
	else {
		/* 其实 else 分支完全可以处理 tail == edit 的情况 */
		char  printbuf[COMMANDLINE_MAX_LEN*2]={0};
		char *tail = &shellin->cmdline[shellin->tail++];
		char *edit = &shellin->cmdline[shellin->edit++];
		char *print = printbuf + (tail - edit);
		char *printend = print + 1;

		/* 当输入过左右箭头时，需要作字符串插入右移处理，并作反馈回显
		   如 abcd 中在bc插入U，需要右移cd，并打印两个 '\b' 使光标回到 abU 处 */
		for (char *cp = tail - 1; cp >= edit ; *tail-- = *cp--) {
			*print-- = *cp;
			*printend++ = '\b';
		}

		/* 插入字符 */
		*print = ascii; 
		*edit  = ascii;
		shellin->cmdline[shellin->tail] = 0 ;
		printl(printbuf,printend - printbuf);
	}
}



/**
  * @author   古么宁
  * @brief    命令行解析输入
  * @param    cmdroot : 检索根，起始检索点
  * @param    shellin : 输入交互
  * @return   don't care
*/
static void shell_parse(cmd_root_t * cmdroot , struct shell_input * shellin)
{
	union uncmd unCmd ;
	unsigned int len = 0;
	unsigned int sum = 0;
	unsigned int fcrc8 = 0;
	unsigned int bcrc8 = 0;
	char  *  str = shellin->cmdline;
	struct shellcommand * cmdmatch;

	/* Shave off any leading spaces */
	for ( ; ' ' == *str ; ++str) ;	

	if (0 == *str)
		goto PARSE_END;

	unCmd.part.FirstChar = *str ;
	for (; (*str) && (*str != ' ') ; ++str ,++len) {
		sum += *str;
		fcrc8 = F_CRC8_Table[fcrc8^*str];
		bcrc8 = B_CRC8_Table[bcrc8^*str];
	}

	unCmd.part.Len = len;
	unCmd.part.Sum = sum;
	unCmd.part.CRC1 = fcrc8;
	unCmd.part.CRC2 = bcrc8;

	cmdmatch = shell_search_cmd(cmdroot,unCmd.ID);
	if (cmdmatch != NULL) {
		/* 判断是否为有选项的命令 */
		shellcfm_t * confirm ;
		confirm = container_of(cmdmatch, struct shellconfirm, cmd);
		if (confirm->flag == CONFIRM_FLAG) {
			shell_confirm(shellin,confirm->prompt,cmdmatch->func);
		}
		else {
			cmdmatch->func(shellin->cmdline);
		}
	}
	else {
		printk("\r\n\tno reply:%s\r\n",shellin->cmdline);
	}
	
PARSE_END:
	shellin->tail = 0;//清空当前命令行输入
	shellin->edit = 0;
	return ;
}

/**
  * @brief    控制台清屏
  * @param    arg  : 命令行内存
  * @return   don't care
*/
static void shell_clean_screen(void * arg)
{
	struct shell_input * shellin ; 
	shellin = container_of(arg, struct shell_input, cmdline);
	printk("\033[2J\033[%d;%dH%s",0,0,shellin->sign);
	return ;
}



/**
  * @author   古么宁
  * @brief    显示所有注册了的命令
  * @param    arg  : 命令行内存
  * @return   don't care
*/
static void shell_list_cmd(void * arg)
{
	struct shell_input * shellin ;
	struct shellcommand * cmd;
	unsigned int firstchar = 0;
	cmd_entry_t  * node ;
	
	for (node = FIRST(&shellcmdroot) ; node; node = NEXT(node)){
		cmd = container_of(node,struct shellcommand, node);
		if (firstchar != (cmd->ID & 0xfc000000)) {
			firstchar = cmd->ID & 0xfc000000;
			printk("\r\n(%c)\t----",((firstchar>>26)|0x40));
		}
		printk("\t%s", cmd->name);
	}

	shellin = container_of(arg, struct shell_input, cmdline);
	printk("\r\n\r\n%s",shellin->sign);
}

/**
  * @author   古么宁
  * @brief    显示所有注册了的命令
  * @param    arg  : 命令行内存
  * @return   don't care
*/
void shell_version(void * arg)
{
	printk("\r\n\t%s\r\n",VERSION);
}

/**
  * @brief  获取 debug 信息
  * @param  arg  : 命令行内存
  * @return don't care
*/
static void shell_debug_stream(void * arg)
{
	static const char closemsg[] = "\r\n\tclose debug information stream\r\n\r\n";
	static const char openmsg[] = "\r\n\tget debug information\r\n\r\n";
	int option;
	int argc = cmdline_param(arg,&option,1);
	
	if ((argc > 0) && (option == 0)) { 
		/* 关闭调试信息打印流，仅显示交互信息 */
		current_puts((char*)closemsg,sizeof(closemsg) - 1);
		default_puts = NULL;  
	}
	else {
		/* 设置当前交互为信息流输出 */
		current_puts((char*)openmsg,sizeof(openmsg) - 1);
		default_puts = current_puts;
	}
}

/**
  * @author   古么宁
  * @brief    注册一个命令号和对应的命令函数 
  * @note     前缀为 '_' 表示不建议直接调用此函数
  * @param    cmd_name : 命令名
  * @param    cmd_func : 命令名对应的执行函数
  * @param    newcmd   : 命令控制块对应的指针
  * @param    confirm  : 命令是否需要确认信息
  * @return   don't care
*/
void _shell_register(struct shellcommand * newcmd,char * cmd_name, cmd_fn_t cmd_func)
{
	char * str = cmd_name;
	union uncmd unCmd ;
	unsigned int clen;
	unsigned int fcrc8 = 0;
	unsigned int bcrc8 = 0;
	unsigned int sum = 0;

	for (clen = 0; *str ; ++clen,++str) {
		sum += *str;
		fcrc8 = F_CRC8_Table[fcrc8^*str];
		bcrc8 = B_CRC8_Table[bcrc8^*str];
	}

	unCmd.part.CRC1 = fcrc8;
	unCmd.part.CRC2 = bcrc8;
	unCmd.part.Len = clen;
	unCmd.part.Sum = sum;
	unCmd.part.FirstChar = *cmd_name;
	
	newcmd->ID = unCmd.ID;   //生成命令码
	newcmd->name = cmd_name;
	newcmd->func = cmd_func;
	shell_insert_cmd(&shellcmdroot,newcmd);
}

/**
  * @author   古么宁
  * @brief    把 "a b c d" 格式化提取为 char*argv[] = {"a","b","c","d"};
  * @note     一般供 getopt() 解析，运行过后命令行内容将被整改
  * @param    str    : 命令字符串后面所跟参数缓冲区指针
  * @param    argv   : 数据转换后缓存地址
  * @param    maxread: 最大读取数
  * @return   最终读取参数个数输出
*/
int cmdline_strtok(char * str ,char ** argv ,int maxread)
{
	int argc = 0;

	for ( ; ' ' == *str ; ++str) ; //跳过空格
	
	for ( ; *str && argc < maxread; ++argc,++argv ) { //字符不为 ‘\0' 的时候
	
		for (*argv = str ; ' ' != *str && *str ; ++str);//记录这个参数，然后跳过非空字符
		
		for ( ; ' ' == *str; *str++ = '\0');//每个参数加字符串结束符，跳过空格		
	}
	
	return argc;
}


/**
  * @brief    转换获取命令号后面的输入参数，字符串转为整数
  * @param    str     命令字符串后面所跟参数缓冲区指针
  * @param    argv    数据转换后缓存地址
  * @param    maxread 最大读取数
  * @return   数据个数
	  * @retval   >= 0         读取命令后面所跟参数个数
	  * @retval   PARAMETER_ERROR(-2)  命令后面所跟参数有误
	  * @retval   PARAMETER_HELP(-1)   命令后面跟了 ? 号
*/
int cmdline_param(char * str,int * argv,int maxread)
{
	int argc ;
	unsigned int  value;

	for ( ; ' ' == *str        ; ++str);//跳过空格
	for ( ; ' ' != *str && *str; ++str);//跳过第一个参数
	for ( ; ' ' == *str        ; ++str);//跳过空格

	if (*str == '?')
		return PARAMETER_HELP;//如果命令后面的是问号，返回help

	for (argc = 0; *str && argc < maxread; ++argc , ++argv) { //字符不为 ‘\0' 的时候
	
		*argv = 0;
		
		if ('0' == str[0] && 'x' == str[1]) { //"0x" 开头，十六进制转换
			for ( str += 2 ;  ; ++str )  {
				if ( (value = *str - '0') < 10 ) // value 先赋值，后判断 
					*argv = (*argv << 4)|value;
				else
				if ( (value = *str - 'A') < 6 || (value = *str - 'a') < 6)
					*argv = (*argv << 4) + value + 10;
				else
					break;
			}
		}
		else { //循环把字符串转为数字，直到字符不为 0 - 9
			unsigned int minus = ('-' == *str);//正负数转换
			if (minus)
				++str;

			for (value = *str - '0'; value < 10 ; value = *(++str) - '0')
				*argv = (*argv * 10 + value);
			
			if (minus)
				*argv = -(*argv);
		}

		if ('\0' != *str && ' ' != *str)//如果不是 0 - 9 而且不是空格，则是错误字符
			return PARAMETER_ERROR;

		for ( ; ' ' == *str ; ++str);//跳过空格,继续判断下一个参数
	}

	return argc;
}

/**
  * @author   古么宁
  * @brief    欢迎页
  * @param    shellin : 交互
  * @param    recv    : 硬件层所接收到的数据缓冲区地址
  * @param    len     : 硬件层所接收到的数据长度
  * @return   don't care
*/
void welcome_gets(struct shell_input * shellin,char * recv,int len)
{
	//打印一个欢迎页logo
    /*
	static const char consolologo[] = "\r\n\
  _____                        __\r\n\
 / ____\\                      /\\ \\\r\n\
/\\ \\___/   ____  ____  ____  _\\_\\ \\     ____\r\n\
\\ \\ \\     / __ \\/ __ \\/ ___\\/ __ \\ \\   / __ \\\r\n\
 \\ \\ \\___L\\ \\L\\ \\ \\/\\ \\____ \\ \\L\\ \\ \\_L\\  ___L\r\n\
  \\ \\______\\____/\\_\\ \\_\\____/\\____/\\____\\____/\r\n\
   \\/______/___/\\/_/\\/_/___/\\/___/\\/____/___/\r\n\
    COPYRIGHT(c):GoodMorning		2019/06\r\n\r\n" ;

	printl((char*)consolologo,sizeof(consolologo)-1);
    */
    static const char help[] = "You can use \"help\" to see the command list.";
    
    printk("\033[1;40;32m");          //设置超级终端背景为黑色，字符为绿色
    printk(help);

    
	shellin->gets = cmdline_gets;
	cmdline_gets(shellin,recv,len);
	return ;
}




/**
  * @author   古么宁
  * @brief    硬件上接收到的数据到命令行的传输
  * @param    shellin : 交互
  * @param    recv    : 硬件层所接收到的数据缓冲区地址
  * @param    len     : 硬件层所接收到的数据长度
  * @return   don't care
*/
void cmdline_gets(struct shell_input * shellin,char * recv,int len)
{
	int state = 0 ;

	for (char * end = recv + len ; recv < end ; ++recv) {
		if (0 == state) {
			/* 普通字符计入内存;否则判断特殊功能字符 */
			if (*recv > 0x1F && *recv < 0x7f)
				shell_getchar(shellin,*recv);
			else
			switch (*recv) {
			case KEYCODE_ENTER:
				if (shellin->tail){
					printk("\r\n");
					shell_record(shellin);
					shell_parse(&shellcmdroot ,shellin);
				}
				else{
					printk("\r\n%s",shellin->sign);
				}
				break;
			case KEYCODE_ESC :
				state = 1;
				break;
			case KEYCODE_CTRL_C:
				shellin->edit = 0;
				shellin->tail = 0;
				printk("^C\r\n%s",shellin->sign);
				break;
			case KEYCODE_BACKSPACE :
			case 0x7f: /* for putty */
				if (shellin->edit)
					shell_backspace(shellin);
				break;
			case KEYCODE_TAB:
				shell_tab(shellin);
				break;
			default: ;
			}
		}
		else 
		if (1 == state){ 
			/* 判断是否是箭头内容 */
			state = (*recv == '[') ? 2 : 0 ;
		}
		else{
			/* if (2 == state) 响应箭头内容 */
			switch(*recv){  
			case 'A'://上箭头
				shell_show_history(shellin,0);
				break;
			case 'B'://下箭头
				shell_show_history(shellin,1);
				break;
			case 'C'://右箭头
				if ( shellin->tail != shellin->edit)
					printl(&shellin->cmdline[shellin->edit++],1);
				break;
			case 'D'://左箭头
				if (shellin->edit){
					--shellin->edit;
					printl("\b",1);
				}
				break;
			default:;
			} //switch 箭头内容
		} // if (2 == state) //响应箭头内容
	} //for ( ; len && *recv; --len,++recv)
	return ;
}

/**
  * @brief    命令行信息确认，如果输入 y/Y 则执行命令
  * @param    shellin : 交互
  * @param    buf     : 硬件层所接收到的数据缓冲区地址
  * @param    len     : 硬件层所接收到的数据长度
  * @return   don't care
*/
static void confirm_gets(struct shell_input * shellin ,char * buf , int len)
{
	char * option = &shellin->cmdline[COMMANDLINE_MAX_LEN-1];

	if (0 == *option) { //先输入 [Y/y/N/n] ，其他按键无效
		if ('Y' == *buf || 'y' == *buf || 'N' == *buf || 'n' == *buf) {
			*option = *buf;
			printl(buf,1);
		}
	}
	else
	if (KEYCODE_BACKSPACE == *buf) { //回退键
		printl("\b \b",3);
		*option = 0;
	}
	else
	if ('\r' == *buf || '\n' == *buf) {//按回车确定
		cmd_fn_t yestodo = (cmd_fn_t)shellin->apparg;
 		char opt = *option ; 
		
		*option = 0 ;  //shellin->cmdline[COMMANDLINE_MAX_LEN-1] = 0;
		shellin->gets   = cmdline_gets;//数据回归为命令行模式
		shellin->apparg = NULL;

		printl("\r\n",2);

		if ( 'Y' == opt || 'y' == opt) 
			yestodo(shellin->cmdline);
		else
			printk("cancel this operation\r\n");
	}
}

/**
  * @brief    命令行信息确认，如果输入 y/Y 则执行命令
  * @param    shell  : 输入交互
  * @param    info   : 选项信息
  * @param    yestodo: 输入 y/Y 后所需执行的命令
  * @return   don't care
*/
void shell_confirm(struct shell_input * shellin ,char * info ,cmd_fn_t yestodo)
{
	printk("%s [Y/N] ",info);
	shellin->gets = confirm_gets;// 数据流获取至 confirm_gets
	shellin->apparg = yestodo;
	shellin->cmdline[COMMANDLINE_MAX_LEN-1] = 0;
}

/**
  * @author   古么宁
  * @brief    初始化一个 shell 交互，默认输入为 cmdline_gets
  * @param    shellin   : 需要初始化的 shell 交互 
  * @param    shellputs : shell 对应输出，如从串口输出。
  * @param    ...       : 对 gets 和 sign 重定义，如追加 MODIFY_SIGN,"shell>>"
  * @return   don't care
*/
void shell_input_init(struct shell_input * shellin , fmt_puts_t shellputs,...)
{
	unsigned int arg  ;
	char * shellsign = DEFAULT_INPUTSIGN;
	shellgets_t shellgets = welcome_gets;
	
	va_list ap;
	va_start(ap, shellputs); //检测有无新定义 

	arg = va_arg(ap, unsigned int) ;
	for (; MODIFY_MASK == (arg & (~0x0f)) ; arg = va_arg(ap, unsigned int) ) {
		if (MODIFY_SIGN == arg) //如果重定义当前交互的输入标志
			shellsign = va_arg(ap, char*);
		else
		if (MODIFY_GETS == arg) //如果重定义当前交互的输入流向
			shellgets = (shellgets_t)va_arg(ap, void*);
	}

	va_end(ap);

	shellin->tail = 0;
	shellin->edit = 0;
	shellin->puts = shellputs;
	shellin->gets = shellgets;
	shellin->htywrt  = 0;
	shellin->htyread = 0;
	shellin->apparg  = NULL;
	strcpy(shellin->sign, shellsign);
}


/**
  * @author   古么宁
  * @brief    shell 初始化,注册几条基本的命令。允许不初始化。
  * @param    defaultsign : 重定义默认输出标志，为 NULL 则不修改默认标志
  * @param    puts        : printf,printk,printl 的默认输出，如从串口输出，为 NULL 则不打印信息。
  * @return   don't care
*/
void shell_init(char * defaultsign ,fmt_puts_t puts)
{
	if (defaultsign)
		strncpy(DEFAULT_INPUTSIGN,defaultsign,COMMANDLINE_MAX_LEN);
	//strcpy(DEFAULT_INPUTSIGN,defaultsign);

	current_puts = puts ;
	default_puts = puts ;
	
	/* 注册一些基本命令 */
	shell_register_command("help"         ,shell_list_cmd);
	//shell_register_command("shell-version",shell_version);
	shell_register_command("clear"        ,shell_clean_screen);
	shell_register_command("debug-info"   ,shell_debug_stream);
}
#endif
