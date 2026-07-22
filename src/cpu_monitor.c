#include "cpu_monitor.h"

#include <stdio.h>

int read_cpu_times(CpuTimes *times)
{
    FILE *file;

    /*
     * 防止调用者传入空指针。
     */
    if (times == NULL)
    {
        return -1;
    }

    /*
     * 以只读方式打开 /proc/stat。
     */
    file = fopen("/proc/stat", "r");

    if (file == NULL)
    {
        perror("fopen /proc/stat");
        return -1;
    }

    /*
     * /proc/stat 第一行格式：
     *
     * cpu user nice system idle iowait irq softirq steal ...
     *
     * fscanf 会跳过开头的 cpu，
     * 然后依次读取后面的八个数字。
     */
    int matched = fscanf(
        file,
        "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &times->user,
        &times->nice,
        &times->system,
        &times->idle,
        &times->iowait,
        &times->irq,
        &times->softirq,
        &times->steal
    );

    fclose(file);

    /*
     * 正常情况下应当成功读取八个数字。
     */
    if (matched != 8)
    {
        fprintf(stderr, "Failed to parse /proc/stat\n");
        return -1;
    }

    return 0;
}

double calculate_cpu_usage(
    const CpuTimes *previous,
    const CpuTimes *current
)
{
    unsigned long long previous_idle;
    unsigned long long current_idle;

    unsigned long long previous_total;
    unsigned long long current_total;

    unsigned long long total_difference;
    unsigned long long idle_difference;

    if (previous == NULL || current == NULL)
    {
        return -1.0;
    }

    /*
     * idle 和 iowait 都视为 CPU 没有执行实际计算任务的时间。
     */
    previous_idle = previous->idle + previous->iowait;
    current_idle = current->idle + current->iowait;

    /*
     * 计算两次读取时的 CPU 总累计时间。
     */
    previous_total =
        previous->user +
        previous->nice +
        previous->system +
        previous->idle +
        previous->iowait +
        previous->irq +
        previous->softirq +
        previous->steal;

    current_total =
        current->user +
        current->nice +
        current->system +
        current->idle +
        current->iowait +
        current->irq +
        current->softirq +
        current->steal;

    /*
     * 计算两次读取之间增加了多少时间。
     */
    total_difference = current_total - previous_total;
    idle_difference = current_idle - previous_idle;

    /*
     * 防止除以 0。
     */
    if (total_difference == 0)
    {
        return 0.0;
    }

    return
        ((double)(total_difference - idle_difference) /
         (double)total_difference) *
        100.0;
}
