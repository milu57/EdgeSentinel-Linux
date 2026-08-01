#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <time.h>

#include "alert.h"

/*
 * 进程名称缓冲区大小。
 */
#define PROCESS_NAME_LENGTH 256

#define MAX_MONITORED_PROCESSES 8

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
 * 保存一个被监控进程的运行状态。
 *
 * ProcessInfo 只表示某一次读取到的进程信息；
 * MonitoredProcess 则保存程序长期监控该进程时
 * 需要使用的当前状态和上一次采样状态。
 */
typedef struct {
    /*
     * 配置中指定的进程名称。
     */
    char target_name[PROCESS_NAME_LENGTH];

    /*
     * 配置中指定的固定 PID。
     *
     * 0 可以表示没有配置固定 PID，
     * 或者使用 EdgeSentinel 自身 PID。
     */
    int configured_pid;

    /*
     * 当前实际找到并正在监控的 PID。
     *
     * 按名称监控时，目标进程重启后，
     * 这个值可能发生变化。
     */
    int current_pid;

    /*
     * 当前读取到的进程基本信息。
     */
    ProcessInfo info;

    /*
     * 当前进程驻留物理内存，单位为 MiB。
     */
    double memory_mib;

    /*
     * 当前计算得到的进程 CPU 使用率。
     */
    double cpu_usage;

    /*
     * 上一次读取到的进程累计 CPU 时间。
     */
    ProcessCpuTimes previous_cpu_times;

    /*
     * 上一次进程 CPU 采样对应的时间。
     */
    struct timespec previous_cpu_sample_time;

    /*
     * 是否已经保存第一次 CPU 采样。
     */
    int cpu_sample_initialized;

    /*
     * 当前 CPU 使用率是否有效。
     */
    int cpu_usage_valid;

    /*
     * 目标进程当前是否可用。
     */
    int available;

    /*
     * 上一次采样时目标进程是否可用。
     */
    int previous_available;

    /*
     * 是否已经保存第一次可用状态。
     */
    int availability_initialized;

    /*
     * 当前和上一次进程 CPU 告警等级。
     */
    AlertLevel cpu_level;
    AlertLevel previous_cpu_level;
    int cpu_level_initialized;

    /*
     * 当前和上一次进程内存告警等级。
     */
    AlertLevel memory_level;
    AlertLevel previous_memory_level;
    int memory_level_initialized;
} MonitoredProcess;

/*
 * 初始化一个被监控进程对象。
 *
 * target_name：
 *     按名称监控时保存进程名称；
 *     不按名称监控时可以传入空字符串。
 *
 * configured_pid：
 *     配置文件中指定的 PID。
 *
 * current_pid：
 *     当前实际监控的 PID；
 *     还没有找到目标进程时可以为 0。
 *
 * 成功返回 0，失败返回 -1。
 */
int monitored_process_init(
    MonitoredProcess *process,
    const char *target_name,
    int configured_pid,
    int current_pid
);

/*
 * 重置目标进程的运行时监控状态。
 *
 * 保留：
 *     target_name
 *     configured_pid
 *     current_pid
 *
 * 清除：
 *     进程信息
 *     CPU 和内存数据
 *     上一次 CPU 采样
 *     可用状态
 *     告警等级及初始化标志
 */
void monitored_process_reset_runtime_state(
    MonitoredProcess *process
);

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


int find_process_by_name(const char *process_name, int *pid);

void monitored_process_reset_cpu_sampling(
    MonitoredProcess *process
);

#endif
