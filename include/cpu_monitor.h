#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

/*
 * 保存 /proc/stat 中 CPU 的累计运行时间。
 *
 * unsigned long long：
 * 这些时间会从系统启动开始不断累加，
 * 数值可能很大，因此不能使用普通 int。
 */
typedef struct
{
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} CpuTimes;

/*
 * 从 /proc/stat 中读取 CPU 累计时间。
 *
 * 参数：
 *     times：用于保存读取结果的结构体地址
 *
 * 返回值：
 *     0  表示读取成功
 *    -1  表示读取失败
 */
int read_cpu_times(CpuTimes *times);

double calculate_cpu_usage(
    const CpuTimes *previous,
    const CpuTimes *current
);

#endif
