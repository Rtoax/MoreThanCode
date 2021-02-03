/**********************************************************************************************************************\
*  文件： fastq.h
*  介绍： 低时延队列
*  作者： 荣涛
*  日期：
*       2021年1月25日    创建与开发轮询功能
*       2021年1月27日 添加 通知+轮询 功能接口，消除零消息时的 CPU100% 问题
*       2021年1月28日 调整代码格式，添加必要的注释
*       2021年2月1日 添加多入单出队列功能
*       2021年2月2日 消息队列句柄改成 unsigned long 索引
*       2021年2月3日 统计类接口 和 低时延接口并存
*
* API接口概述
*   
*   VOS_FastQCreateModule   注册消息队列
*   VOS_FastQDump           显示信息
*   VOS_FastQSend           发送消息（轮询直至成功发送）
*   VOS_FastQTrySend        发送消息（尝试向队列中插入，当队列满是直接返回false）
*   VOS_FastQRecv           接收消息
*   
\**********************************************************************************************************************/

#ifndef __fAStMQ_H
#define __fAStMQ_H 1

#include <stdbool.h>

#ifndef always_inline
#define always_inline __attribute__ ((__always_inline__))
#endif


/**
 *  源模块未初始化时可临时使用的模块ID，只允许使用一次 
 */
#define VOS_FastQTmpModuleID    0 

/**
 *  fq_msg_handler_t - FastQRecvMain 接收函数
 *  
 *  param[in]   msg 接收消息地址
 *  param[in]   sz 接收消息大小，与 FastQCreate (..., msg_size) 保持一致
 */
typedef void (*fq_msg_handler_t)(void*msg, size_t sz);


/**
 *  VOS_FastQCreateModule - 注册消息队列
 *  
 *  param[in]   moduleID    模块ID， 范围 1 - FASTQ_ID_MAX
 *  param[in]   msgMax      该模块 的 消息队列 的大小
 *  param[in]   msgSize     最大传递的消息大小
 */
always_inline void 
VOS_FastQCreateModule(const unsigned long moduleID, const unsigned int msgMax, const unsigned int msgSize, 
                            const char *_file, const char *_func, const int _line);

/**
 *  VOS_FastQDump - 显示信息
 *  
 *  param[in]   fp    文件指针
 *  param[in]   module_id 需要显示的模块ID， 等于 0 时显示全部
 */
always_inline void 
VOS_FastQDump(FILE*fp, unsigned long moduleID);

/**
 *  VOS_FastQDump - 显示全部模块信息
 *  
 *  param[in]   fp    文件指针
 */
always_inline void 
VOS_FastQDumpAllModule(FILE*fp);


/**
 *  VOS_FastQSend - 发送消息（轮询直至成功发送）
 *  
 *  param[in]   from    源模块ID， 范围 1 - FASTQ_ID_MAX 
 *  param[in]   to      目的模块ID， 范围 1 - FASTQ_ID_MAX
 *  param[in]   msg     传递的消息体
 *  param[in]   size    传递的消息大小
 *
 *  return 成功true （轮询直至发送成功，只可能返回 true ）
 *
 *  注意：from 和 to 需要使用 FastQCreateModule 注册后使用
 */
always_inline bool 
VOS_FastQSend(unsigned int from, unsigned int to, const void *msg, size_t size);


/**
 *  VOS_FastQTrySend - 发送消息（尝试向队列中插入，当队列满是直接返回false）
 *  
 *  param[in]   from    源模块ID， 范围 1 - FASTQ_ID_MAX 
 *  param[in]   to      目的模块ID， 范围 1 - FASTQ_ID_MAX
 *  param[in]   msg     传递的消息体
 *  param[in]   size    传递的消息大小
 *
 *  return 成功true 失败false
 *
 *  注意：from 和 to 需要使用 FastQCreateModule 注册后使用
 */
always_inline bool 
VOS_FastQTrySend(unsigned int from, unsigned int to, const void *msg, size_t size);

/**
 *  VOS_FastQRecv - 接收消息
 *  
 *  param[in]   from    从模块ID from 中读取消息， 范围 1 - FASTQ_ID_MAX 
 *  param[in]   handler 消息处理函数，参照 fq_msg_handler_t 说明
 *
 *  return 成功true 失败false
 *
 *  注意：from 需要使用 FastQCreateModule 注册后使用
 */
always_inline  bool 
VOS_FastQRecv(unsigned int from, fq_msg_handler_t handler);



/**********************************************************************************************************************\
 **
 **                      从此至该头文件末尾，所有接口禁止应用层使用 
**
\**********************************************************************************************************************/
    
#define FastQTmpModuleID    VOS_FastQTmpModuleID 


#ifdef _FASTQ_STATS /* 带有统计类的接口 */
//# pragma message "[FastQ] Statistic Class API"
# define VOS_FastQCreateModule(moduleID, msgMax, msgSize)    FastQCreateModuleStats(moduleID, msgMax, msgSize, __FILE__, __func__, __LINE__)
# define VOS_FastQDump(fp, moduleID)                            FastQDumpStats(fp, moduleID)
# define VOS_FastQDumpAllModule(fp)                            FastQDumpStats(fp, 0)
# define VOS_FastQSend(moduleSrc, moduleDst, pmsg, msgSize)  FastQSendStats(moduleSrc, moduleDst, pmsg, msgSize)  
# define VOS_FastQTrySend(moduleSrc, moduleDst, pmsg, msgSize)  FastQTrySendStats(moduleSrc, moduleDst, pmsg, msgSize)  
# define VOS_FastQRecv(fromModule, msgHandlerFn)             FastQRecvStats(fromModule, msgHandlerFn)

#else /* 不带有统计类的接口，时延更低 */
//# pragma message "[FastQ] Low Latency Class API"
# define VOS_FastQCreateModule(moduleID, msgMax, msgSize)    FastQCreateModule(moduleID, msgMax, msgSize, __FILE__, __func__, __LINE__)
# define VOS_FastQDump(fp, moduleID)                            FastQDump(fp, moduleID)
# define VOS_FastQDumpAllModule(fp)                            FastQDump(fp, 0)
# define VOS_FastQSend(moduleSrc, moduleDst, pmsg, msgSize)  FastQSend(moduleSrc, moduleDst, pmsg, msgSize)  
# define VOS_FastQTrySend(moduleSrc, moduleDst, pmsg, msgSize)  FastQTrySend(moduleSrc, moduleDst, pmsg, msgSize)  
# define VOS_FastQRecv(fromModule, msgHandlerFn)             FastQRecv(fromModule, msgHandlerFn)

#endif


/**
 *  FastQCreateModule - 注册消息队列
 *  
 *  param[in]   moduleID    模块ID， 范围 1 - FASTQ_ID_MAX
 *  param[in]   msgMax      该模块 的 消息队列 的大小
 *  param[in]   msgSize     最大传递的消息大小
 */
always_inline void 
FastQCreateModule(const unsigned long moduleID, const unsigned int msgMax, const unsigned int msgSize, 
                            const char *_file, const char *_func, const int _line);
always_inline void 
FastQCreateModuleStats(const unsigned long moduleID, const unsigned int msgMax, const unsigned int msgSize, 
                            const char *_file, const char *_func, const int _line);


/**
 *  FastQDump - 显示信息
 *  
 *  param[in]   fp    文件指针
 *  param[in]   module_id 需要显示的模块ID， 等于 0 时显示全部
 */
always_inline void 
FastQDump(FILE*fp, unsigned long module_id);
always_inline void 
FastQDumpStats(FILE*fp, unsigned long module_id);

/**
 *  FastQSend - 发送消息（轮询直至成功发送）
 *  
 *  param[in]   from    源模块ID， 范围 1 - FASTQ_ID_MAX 
 *  param[in]   to      目的模块ID， 范围 1 - FASTQ_ID_MAX
 *  param[in]   msg     传递的消息体
 *  param[in]   size    传递的消息大小
 *
 *  return 成功true （轮询直至发送成功，只可能返回 true ）
 *
 *  注意：from 和 to 需要使用 FastQCreateModule 注册后使用
 */
always_inline bool 
FastQSend(unsigned int from, unsigned int to, const void *msg, size_t size);
always_inline bool 
FastQSendStats(unsigned int from, unsigned int to, const void *msg, size_t size);


/**
 *  FastQTrySend - 发送消息（尝试向队列中插入，当队列满是直接返回false）
 *  
 *  param[in]   from    源模块ID， 范围 1 - FASTQ_ID_MAX 
 *  param[in]   to      目的模块ID， 范围 1 - FASTQ_ID_MAX
 *  param[in]   msg     传递的消息体
 *  param[in]   size    传递的消息大小
 *
 *  return 成功true 失败false
 *
 *  注意：from 和 to 需要使用 FastQCreateModule 注册后使用
 */
always_inline bool 
FastQTrySend(unsigned int from, unsigned int to, const void *msg, size_t size);
always_inline bool 
FastQTrySendStats(unsigned int from, unsigned int to, const void *msg, size_t size);

/**
 *  FastQRecv - 接收消息
 *  
 *  param[in]   from    从模块ID from 中读取消息， 范围 1 - FASTQ_ID_MAX 
 *  param[in]   handler 消息处理函数，参照 fq_msg_handler_t 说明
 *
 *  return 成功true 失败false
 *
 *  注意：from 需要使用 FastQCreateModule 注册后使用
 */
always_inline  bool 
FastQRecv(unsigned int from, fq_msg_handler_t handler);
always_inline  bool 
FastQRecvStats(unsigned int from, fq_msg_handler_t handler);


#endif /*<__fAStMQ_H>*/
