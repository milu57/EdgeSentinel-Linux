#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "cpu_monitor.h"
#include "system_monitor.h"

/*
 * 控制主循环是否继续运行。
 *
 * volatile：
 * 防止编译器假设这个变量不会突然变化。
 *
 * sig_atomic_t：
 * 适合在信号处理函数和普通程序之间传递简单状态。
 */
static volatile sig_atomic_t keep_running = 1;

/*
 * 用户按下 Ctrl+C 后，系统调用这个函数。
 */
static void handle_sigint(int signal_number)
{
    /*
     * 当前不使用 signal_number，
     * 显式转换为 void 可消除编译器警告。
     */
    (void)signal_number;

    /*
     * 不在信号处理函数中执行复杂操作，
     * 只通知主循环停止。
     */
    keep_running = 0;
}

int main(void)
{
    struct sigaction action;

    CpuTimes previous_cpu;
    CpuTimes current_cpu;

    MemoryInfo memory_info;

    double cpu_usage;

    /*
     * 配置 SIGINT，也就是 Ctrl+C 的处理方式。
     */
    action.sa_handler = handle_sigint;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    /*
     * 注册信号处理函数。
     */
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    /*
     * CPU 使用率需要比较两次累计数据，
     * 所以程序启动时先取得第一份数据。
     */
    if (read_cpu_times(&previous_cpu) != 0)
    {
        fprintf(stderr, "Failed to read initial CPU times\n");
        return 1;
    }

    printf("EdgeSentinel system monitor started.\n");
    printf("Press Ctrl+C to stop.\n\n");

    while (keep_running)
    {
        /*
         * 等待一秒，形成 CPU 统计区间。
         */
        sleep(1);

        /*
         * Ctrl+C 可能在 sleep 期间发生，
         * 所以 sleep 返回后重新检查循环状态。
         */
        if (!keep_running)
        {
            break;
        }

        /*
         * 读取第二份 CPU 累计时间。
         */
        if (read_cpu_times(&current_cpu) != 0)
        {
            fprintf(stderr, "Failed to read current CPU times\n");
            return 1;
        }

        /*
         * 根据前后两次数据计算这一秒内的 CPU 使用率。
         */
        cpu_usage = calculate_cpu_usage(
            &previous_cpu,
            &current_cpu
        );

        if (cpu_usage < 0.0)
        {
            fprintf(stderr, "Failed to calculate CPU usage\n");
            return 1;
        }

        /*
         * 从 /proc/meminfo 读取当前内存信息。
         *
         * 内存使用率不需要读取两次，
         * 因为它描述的是当前时刻的内存状态。
         */
        if (get_memory_info(&memory_info) != 0)
        {
            fprintf(stderr, "Failed to read memory information\n");
            return 1;
        }

        printf("CPU Usage:    %.2f%%\n", cpu_usage);
        printf("Memory Usage: %.2f%%\n", memory_info.usage_percent);
        printf("----------------------------\n");

        /*
         * 本轮 CPU 数据作为下一轮的旧数据。
         */
        previous_cpu = current_cpu;
    }

    printf("\nEdgeSentinel stopped safely.\n");

    return 0;
}

