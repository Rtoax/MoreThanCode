/**
 *  FastLog 低时延/高吞吐 LOG日志系统
 *
 *  Author: Rong Tao
 *  data: 2021年6月2日 - 
 */

#ifndef __fAStLOG_H
#define __fAStLOG_H 1

#include <fastlog_internal.h>


/* 级别 */
enum FASTLOG_LEVEL
{
    FASTLOGLEVEL_ALL = 0,
	FASTLOG_CRIT = 1,
	FASTLOG_ERR,      
	FASTLOG_WARNING, 
	FASTLOG_INFO,      
	FASTLOG_DEBUG,     
	FASTLOGLEVELS_NUM  
};

/**
 *  FastLog API
 *
 *  level   日志级别(enum FASTLOG_LEVEL)
 *  name    用户字符串(模块名，prefix等)
 *  format  printf(fmt, ...)'s fmt
 *  ...     printf(fmt, ...)'s ...
 *
 *  这其实是一个宏定义
 */
void FAST_LOG(int level, char *name, char *format, ...);


/**
 *  FastLog API
 *
 *  level   日志级别(enum FASTLOG_LEVEL)
 *  
 *  在终端下使用`strlevel_color`可以显示带有颜色的日志级别
 */
const char *strlevel(enum FASTLOG_LEVEL level);
const char *strlevel_color(enum FASTLOG_LEVEL level);


/**
 *  FastLog 初始化
 *
 *  level           日志激活级别, 可用`fastlog_setlevel`设置
 *  nr_logfile      最大日志文件数
 *  log_file_size   单个日志文件大小(bytes)
 *  cpu             后台线程绑核操作
 */
void fastlog_init(enum FASTLOG_LEVEL level, size_t nr_logfile, size_t log_file_size, int cpu);
void fastlog_exit();

/**
 *  日志级别的激活与去激活
 *  输出级别大于等于这个级别的日志(不是枚举值，而知 critical->error->warning->notice->debug 这个级别)
 *
 *  注意不要使用 `FASTLOGLEVEL_ALL` 作为 入参，这将关闭所有日志
 */
void fastlog_setlevel(enum FASTLOG_LEVEL level);
enum FASTLOG_LEVEL fastlog_getlevel();


#define FAST_LOG(level, name, format, ...) __FAST_LOG(level, name, format, ##__VA_ARGS__)


#endif /*<__fAStLOG_H>*/
