/**
 *  FastLog 低时延 LOG日志
 *
 *  代码日志
 *  2021年5月21日    创建源码文件
 *
 */

#define __USE_GNU
#define _GNU_SOURCE
#include <sched.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>

#include <fastlog.h>

#ifndef _NR_CPU
#define _NR_CPU    32
#endif

#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)
#define __cachelinealigned __attribute__((aligned(64)))
#define _unused             __attribute__((unused))



struct fastlog_operation;

struct fastlog {
    
    int level;
    int enable;
    
    struct fastlog_operation *fastlog_op;

    void *private_data;
}__cachelinealigned;



struct fastlog_operation {
    int (*open)(struct fastlog *fl, int level);
    int (*enable)(struct fastlog *fl);
    int (*disable)(struct fastlog *fl);
    int (*log)(struct fastlog *fl, int level, char *fmt, ...);
    int (*close)(struct fastlog *fl);
};





#define DEFINE_PERFCPU_FASTLOG(name) \
    struct fastlog name[_NR_CPU]
    
#define PERCPU_FASTLOG(name) \
    (struct fastlog*)name[__getcpu()]

DEFINE_PERFCPU_FASTLOG(fastlogs);

static void chk_process() 
{
    int n_cpu = sysconf (_SC_NPROCESSORS_ONLN);
    if(_NR_CPU < n_cpu) {
        printf("Macro _NR_CPU(%d) Must bigger than _SC_NPROCESSORS_ONLN(%d).\n", _NR_CPU, n_cpu);
        assert(0);
    }
}

static void __attribute__((constructor(105))) __fastlog_init() 
{
    chk_process();

    return true;
}


static inline int log_file_open(struct fastlog *fl, int level) 
{

    return 0;
}


static inline int log_file_enable(struct fastlog *fl)
{

    return 0;
}

static inline int log_file_disable(struct fastlog *fl)
{

    return 0;
}

static inline int log_file_log(struct fastlog *fl, int level, char *fmt, ...)
{

    return 0;
}

static inline int log_file_close(struct fastlog *fl)
{

    return 0;
}


static struct fastlog_operation log_file_ops = {
    .open = log_file_open,
    .enable = log_file_enable,
    .disable = log_file_disable,
    .log = log_file_log,
    .close = log_file_close,
};
