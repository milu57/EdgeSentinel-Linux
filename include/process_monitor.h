#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

/*
 * 进程名称缓冲区大小。
 */
#define PROCESS_NAME_LENGTH 256

/*
 * 进程状态字符串缓冲区大小。
 */
#define PROCESS_STATE_LENGTH 64

/*
 * 保存一个进程的基本信息。
 */
typedef struct {
    int pid;
    int parent_pid;
    char name[PROCESS_NAME_LENGTH];
    char state[PROCESS_STATE_LENGTH];
    unsigned long resident_memory_kb;
} ProcessInfo;

/*
 * 保存一个进程的累计 CPU 时间。
 *
 * 单位不是秒，而是 clock ticks。
 */
typedef struct {
    unsigned long long user_ticks;
    unsigned long long system_ticks;
} ProcessCpuTimes;

/*
 * 从 /proc/<pid>/status 中读取进程基本信息。
 */
int read_process_info(
    int pid,
    ProcessInfo *info
);

/*
 * 从 /proc/<pid>/stat 中读取进程累计 CPU 时间。
 */
int read_process_cpu_times(
    int pid,
    ProcessCpuTimes *times
);

/*
 * 根据两次累计 CPU 时间计算进程 CPU 使用率。
 */
double calculate_process_cpu_usage(
    const ProcessCpuTimes *previous,
    const ProcessCpuTimes *current,
    double elapsed_seconds
);

#endif
