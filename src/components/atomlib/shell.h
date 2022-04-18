/**
  ******************************************************************************
  * @file           shell.h
  * @author         古么宁
  * @brief          命令解释器头文件
  * 使用步骤：
  * <pre>
  * 使用步骤：
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
#ifndef __SHELL_H__
#define __SHELL_H__

// 以下为 shell 所依赖的基本库
#include "ustdio.h"

/* Public macro (共有宏)------------------------------------------------------------*/

/* ---- option (配置项) ---- */

/**
 * @brief 为 1 时使能平衡二叉树进行查找匹配，否则使用单链表。
 * @note  用二叉树构建查询系统要比链表快，但需要加入 avltree.c 文件支持，会
 *        多编译大概 800~1000bytes 的 ROM 占用。适用于注册的命令较多时启用。 
*/
#define USE_AVL_TREE            0

/// 命令带上参数的字符串输入最长记录长度
#define COMMANDLINE_MAX_LEN     36

/// 控制台记录条目数，设为 0 时不记录
#define COMMANDLINE_MAX_RECORD  8

/* ---- option end ---- */

// 一些键值：
#define KEYCODE_END               35
#define KEYCODE_HOME              36
#define KEYCODE_CTRL_C            0x03
#define KEYCODE_BACKSPACE         0x08   //键盘的回退键
#define KEYCODE_TAB               '\t'   //键盘的tab键
#define KEYCODE_NEWLINE           0x0A
#define KEYCODE_ENTER             0x0D   //键盘的回车键
#define KEYCODE_ESC               0x1b


#define MODIFY_MASK  0xABCD4320
#define FUNC_NAME(_1, _2, _3, NAME,...) NAME
#define FUNC(...)                       FUNC_NAME(__VA_ARGS__,PRINTF3,PRINTF2)(__VA_ARGS__)


/**
  * @author   古么宁
  * @brief    往控制台注册命令
  * @note     调用宏注册命令的同时会新建一个与命令对应的控制块
  * @param    name  : 名称，必须为常量字符串指针
  * @param    func  : 命令执行函数，@see cmd_fn_t
*/
#define shell_register_command(name,func)\
do{\
	static struct shellcommand newcmd = {0};\
	_shell_register(&newcmd,name,func);     \
}while(0)


/**
  * @author   古么宁
  * @brief    往控制台注册一个带选项命令，需要输入 [Y/N/y/n] 才执行对应的命令
  * @note     调用宏注册命令的同时会新建一个与命令对应的控制块
  * @param    name  : 名称，必须为常量字符串指针
  * @param    func  : 命令执行函数，@see cmd_fn_t
*/
#define shell_register_confirm(name,func,info)\
do{\
	static struct shellconfirm confirm = {  \
	.prompt = info ,.flag = CONFIRM_FLAG }; \
	_shell_register(&confirm.cmd,name,func);\
}while(0)


/// 以下为 shell_input_init() 所用宏
#define MODIFY_SIGN (MODIFY_MASK|0x1)
#define MODIFY_GETS (MODIFY_MASK|0x2)

/// 历史遗留问题，兼容旧版本代码
#define SHELL_INPUT_INIT(...) shell_input_init(__VA_ARGS__)


/// shell 入口对应出口，从哪里输入则从对应的地方输出
#define shell_input(shellin,buf,len) \
do{\
	if ((shellin)->gets) {\
		current_puts = (shellin)->puts;        \
		(shellin)->gets((shellin),(buf),(len));\
		current_puts = default_puts;           \
	}\
}while(0)


/* Public types ------------------------------------------------------------*/

enum INPUT_PARAMETER {
	PARAMETER_ERROR = -2,
	PARAMETER_HELP = -1,
};


/// 命令对应的函数类型，至于为什么输入设计为 void *,我不记得了
typedef void (*cmd_fn_t)(void * arg);


#if USE_AVL_TREE     // 命令索引用avl树进行查找匹配时需要 avltree.c 支持
	#include "avltree.h"
	typedef struct avl_node cmd_entry_t ;
	typedef struct avl_root cmd_root_t ;
#else                   // 单链表节点，用来串命令
	struct slist{struct slist * next;} ;
	typedef struct slist cmd_entry_t ;
	typedef struct slist cmd_root_t ;
#endif


/// 命令结构体，用于注册匹配命令
typedef struct shellcommand {
	cmd_entry_t   node    ; ///< 命令索引接入点，用链表或二叉树对命令作集合
	char *        name    ; ///< 记录每条命令字符串的内存地址
	cmd_fn_t      func    ; ///< 记录命令函数 cmd_fn_t 对应的内存地址
	unsigned int  ID      ; ///< 对 name 字符串进行压缩得到的 ID 号，匹配数字比字符串效率高。
}
shellcmd_t;


/// 带确认选项的命令结构体
typedef struct shellconfirm {
	struct shellcommand  cmd; /// 对应的命令号内存
	char * prompt ;
	#define CONFIRM_FLAG 0x87654321U
	size_t flag   ;           /// 确认提示信息
}
shellcfm_t ;

/// 交互结构体，数据的输入输出不一定
typedef struct shell_input
{
	/// 指定数据流输入,初始化默认为 cmdline_gets() ,即命令行
	void (*gets)(struct shell_input * , char * ,int );

	/// 指定数据流对应的输出接口，串口或者 telnet 输出等
	fmt_puts_t puts;

	/// app可用参数，爱怎么用就怎么用
	void *  apparg;

	/// 命令行输入符号
	char    sign[COMMANDLINE_MAX_LEN];

	// 命令行相关的参数
	char          cmdline[COMMANDLINE_MAX_LEN]; ///< 命令行内存
	unsigned char edit                        ; ///< 当前命令行编辑位置
	unsigned char tail                        ; ///< 当前命令行输入结尾 tail

	#if (COMMANDLINE_MAX_RECORD) //如果定义了历史纪录
		unsigned char htywrt  ;  ///< 历史记录写
		unsigned char htyread ; ///< 历史记录读
		char    history[COMMANDLINE_MAX_RECORD][COMMANDLINE_MAX_LEN]; ///< 历史记录内存
	#endif
}
shellinput_t;


typedef	void (*shellgets_t)(struct shell_input * , char * ,int );

/* Public variables ---------------------------------------------------------*/

extern char DEFAULT_INPUTSIGN[]; // 默认交互标志


/* Public function prototypes 对外可用接口 -----------------------------------*/

//注册命令，这个函数一般不直接调用，用宏 shell_register_command() 间接调用
void _shell_register(struct shellcommand * newcmd,char * cmd_name, cmd_fn_t cmd_func);


/**
  * @author   古么宁
  * @brief    硬件上接收到的数据到命令行的传输
  * @param    shellin : 交互
  * @param    recv    : 硬件层所接收到的数据缓冲区地址
  * @param    len     : 硬件层所接收到的数据长度
  * @return   void
*/
void cmdline_gets(struct shell_input * ,char * ,int );

//解析命令行参数相关功能函数
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
int  cmdline_param(char * str,int * argv,int maxread);

/**
  * @author   古么宁
  * @brief    把 "a b c d" 格式化提取为 char*argv[] = {"a","b","c","d"};以供getopt()解析
  * @param    str    : 命令字符串后面所跟参数缓冲区指针
  * @param    argv   : 数据转换后缓存地址
  * @param    maxread: 最大读取数
  * @return   最终读取参数个数输出
*/
int  cmdline_strtok(char * str ,char ** argv ,int maxread);

// 初始化相关函数

/**
  * @author   古么宁
  * @brief    shell 初始化,注册几条基本的命令。允许不初始化。
  * @param    defaultsign : 重定义默认输出标志，为 NULL 则不修改默认标志
  * @param    puts        : printf,printk,printl 的默认输出，如从串口输出，为 NULL 则不打印信息。
  * @return   don't care
*/
void shell_init(char * defaultsign ,fmt_puts_t puts);

/**
  * @author   古么宁
  * @brief    初始化一个 shell 交互，默认输入为 cmdline_gets
  * @param    shellin   : 需要初始化的 shell 交互 
  * @param    shellputs : shell 对应输出，如从串口输出。
  * @param    ...       : 对 gets 和 sign 重定义，如追加 MODIFY_SIGN,"shell>>"
  * @return   don't care
*/
void shell_input_init(struct shell_input * shellin , fmt_puts_t shellputs,...);


/**
  * @brief    命令行信息确认，如果输入 y/Y 则执行命令
  * @param    shell  : 输入交互
  * @param    info   : 选项信息
  * @param    yestodo: 输入 y/Y 后所需执行的命令
  * @return   void
*/
void shell_confirm(struct shell_input * shellin ,char * info ,cmd_fn_t yestodo) ;
#endif
