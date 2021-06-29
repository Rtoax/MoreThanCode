#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <signal.h>

#define __USE_GNU
#include <sched.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

#include <fastlog.h>

#include "common.h"

static void *task_module3(void*param) 
{
    struct task_arg * arg = (struct task_arg *)param;
    
    set_thread_cpu_affinity(arg->cpu);    
   
    unsigned long total_dequeue = 0;

    while(1) {

        total_dequeue += 3;
    
        FAST_LOG(FASTLOG_CRIT, MODULE(MODULE_3), "module 1 %ld\n", total_dequeue);
    
        
        sleep(1);
    }
    pthread_exit(arg);
}

void module3_init()
{
    int ncpu = sysconf (_SC_NPROCESSORS_ONLN);
    
    struct task_arg *arg3 = malloc(sizeof(struct task_arg));
    
    arg3->cpu = 2%ncpu;
    
    pthread_create(&module_threads[MODULE_3], NULL, task_module3, arg3);

    pthread_setname_np(module_threads[MODULE_3], modules_name[MODULE_3]);

}




