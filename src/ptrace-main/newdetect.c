#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <signal.h>
#include <time.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <execinfo.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>

#include "associative_array.h"
#include "naming_utils.h"
#include "tracing_utils.h"
#include "die.h"
#include "read_proc.h"

#ifndef __x86_64
#error Only x86-64 is supported now
#endif

#define max(a, b) ((a) > (b) ? (a) : (b))

#define UNUSED(x) x __attribute__((unused))
// 定义了一个存储用户数据的结构体，包括syscall ID和最大内存使用量等信息
struct userdata {
    register_type syscall_id;
    unsigned long max_mem;
};
// 存储页面大小，
struct {
    unsigned long page_size;
} static_data;

//监视内存，会在syscall结束后检查进程的内存使用情况，并且更新‘data->max_mem’的值
void check_memory_usage(pid_t pid, void* data_) {
    struct userdata* data = (struct userdata*)data_;

    struct statm_info statm = get_process_statm_info(pid);
    data->max_mem = max(data->max_mem, statm.size * static_data.page_size);
}
//根据给定的id和函数指针get_name获取对应的名称，并将名称和id组合成一个字符串，存储在buf中。
void combined_name(char* buf, size_t buf_sz, long id, const char* (*get_name)(long)) {
    const char* friendlyname = get_name(id);
    snprintf(buf, buf_sz, "%s{%ld}", (friendlyname == NULL ? "unknown" : friendlyname), id);
}
//定义了处理信号的on_signal函数，当接收到信号时会打印信号的名称和进程ID。
int on_signal(pid_t pid, int sig, void* UNUSED(userptr)) {
    char buf[100];
    combined_name(buf, sizeof(buf), sig, get_signal_name);
    fprintf(stderr, "Signal: %s in %d\n", buf, (int)(pid));
    return (sig != SIGTRAP);
}
//定义了处理系统调用的on_syscall函数，它会在进入系统调用和离开系统调用时打印相关信息，包括系统调用的名称、参数和返回值。
void on_syscall(pid_t pid, int type, void* data_) {
    struct user_regs_struct regs;
    struct syscall_info     info;
    struct userdata*        data = (struct userdata*)(data_);

    extract_registers(pid, &regs);

    if (type == 0) { // enter
        extract_syscall_params(&regs, &info);
        data->syscall_id = info.id;

        char buf[100];
        combined_name(buf, sizeof(buf), info.id, get_syscall_name);

        fprintf(stderr, "Entering syscall %s(%lld, %lld, %lld, %lld, %lld, %lld)\n", buf, info.arg1, info.arg2, info.arg3, info.arg4, info.arg5, info.arg6);
    } else {
        extract_syscall_result(&regs, &info);
        
        if (data->syscall_id == SYS_brk || data->syscall_id == SYS_mmap || data->syscall_id == SYS_munmap)
            check_memory_usage(pid, data_);

        if (info.err == 0)
            fprintf(stderr, "Leaving syscall, result %lld\n", info.ret);
        else
            fprintf(stderr, "Leaving syscall, error %lld\n", info.err);
    }
}

void on_child_exit(pid_t pid, int code, void* data_) {
    struct userdata* data = (struct userdata*)data_;
    fprintf(stderr, "[%d exited with %d]\n", pid, code);
    fprintf(stderr, "max memory used: %lu bytes (%lu kb)\n", data->max_mem, data->max_mem / 1024);
}

void on_groupstop(pid_t pid, void* UNUSED(user_ptr)) {
    fprintf(stderr, "Group stop %d\n", (int)(pid));
}

void on_ptrace_event(pid_t pid, int event, void* UNUSED(user_ptr)) {
    char buf[100];
    combined_name(buf, sizeof(buf), event, get_ptraceevent_name);
    fprintf(stderr, "Ptrace event %s in %d\n", buf, pid);
}
//在main函数中，检查命令行参数是否足够，如果不足则打印错误信息并退出。
//使用fork创建一个子进程，子进程负责执行待跟踪的程序。
//在子进程中，调用trace_me函数以启用跟踪。
//在子进程中，调用execve函数以执行待跟踪的程序。
//在父进程中，初始化静态数据结构static_data，其中包含系统页面大小。
//设置回调函数结构体callbacks，将之前定义的回调函数与相应的事件关联起来。
//调用tracing_loop函数开始跟踪，直到跟踪结束。
//返回0表示程序正常退出。
int main(int argc, char** argv) {
    if (argc < 2)
        die(1, "Not enough args, wanted: >=1, %d supplied\n", argc - 1);

    pid_t child = fork();
    if (child == -1)
        die(1, "Failed to create process\n");
    if (child == 0) {
        // child code.
        trace_me();
        
        execve(argv[1], argv + 1, NULL);
    } else {
        // parent code.
        
        static_data.page_size = getpagesize();

        struct tracing_callbacks callbacks;
        struct userdata data;
        callbacks.on_signal       = on_signal;
        callbacks.on_syscall      = on_syscall;
        callbacks.on_child_exit   = on_child_exit;
        callbacks.on_groupstop    = on_groupstop;
        callbacks.on_ptrace_event = on_ptrace_event;
        
        tracing_loop(&callbacks, &data); 
    }
    
    return 0;
}

