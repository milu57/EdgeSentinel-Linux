#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "system_monitor.h"
#include "system_status.h"

/*
 * 1 GiB = 1024 × 1024 × 1024 字节。
 */
#define BYTES_PER_GIB 1073741824.0

/*
 * 控制程序主循环。
 */
static volatile sig_atomic_t keep_running = 1;

static void handle_sigint(int signal_number)
{
    (void)signal_number;
    keep_running = 0;
}

int main(void)
{
    struct sigaction action;

    CpuTimes previous_cpu;
    CpuTimes current_cpu;

    MemoryInfo memory_info;
    DiskInfo disk_info;
    SystemUptime system_uptime;
    LoadAverage load_average;
    CurrentTime current_time;


    unsigned long long uptime_seconds;
    double cpu_usage;

    /*
     * 配置 Ctrl+C 信号处理。
     */
    action.sa_handler = handle_sigint;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    /*
     * CPU 使用率需要两次采样，
     * 因此启动时先读取一次。
     */
    if (read_cpu_times(&previous_cpu) != 0)
    {
        fprintf(stderr, "Failed to read initial CPU times\n");
        return 1;
    }

    printf("EdgeSentinel system monitor started.\n");
    printf("Monitoring disk mount point: /\n");
    printf("Press Ctrl+C to stop.\n\n");

    while (keep_running)
    {
        sleep(1);

        /*
         * Ctrl+C 可能使 sleep 提前结束。
         */
        if (!keep_running)
        {
            break;
        }

        /*
         * 读取当前 CPU 数据。
         */
        if (read_cpu_times(&current_cpu) != 0)
        {
            fprintf(stderr, "Failed to read current CPU times\n");
            return 1;
        }

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
         * 读取内存数据。
         */
        if (get_memory_info(&memory_info) != 0)
        {
            fprintf(stderr, "Failed to read memory information\n");
            return 1;
        }

        /*
         * 读取根文件系统 / 的磁盘数据。
         */
        if (get_disk_info("/", &disk_info) != 0)
        {
            fprintf(stderr, "Failed to read disk information\n");
            return 1;
        }

	/*
	 * 读取系统启动后经过的总秒数。
	 */
	if (get_system_uptime(&uptime_seconds) != 0)
	{
	    fprintf(stderr, "Failed to read system uptime\n");
	    return 1;
	}

	/*
	 * 将总秒数转换成天、小时、分钟和秒。
	 */
	convert_uptime(
	    uptime_seconds,
	    &system_uptime
	);

	/*
	 * 读取 1、5、15 分钟系统负载。
	 */
	if (get_load_average(&load_average) != 0)
	{
	    fprintf(stderr, "Failed to read load average\n");
	    return 1;
	}

	/*
	 * 获取当前系统本地时间。
	 */
	if (get_current_time(&current_time) != 0)
	{
	    fprintf(stderr, "Failed to get current time\n");
	    return 1;
	}

	printf(
	    "Updated:         %04d-%02d-%02d %02d:%02d:%02d\n",
	    current_time.year,
	    current_time.month,
	    current_time.day,
	    current_time.hour,
	    current_time.minute,
	    current_time.second
	);

	printf(
	    "System Uptime:   %llu days %u hours "
	    "%u minutes %u seconds\n",
	    system_uptime.days,
	    system_uptime.hours,
	    system_uptime.minutes,
	    system_uptime.seconds
	);

	printf(
    	    "Load Average:    %.2f  %.2f  %.2f\n",
    	    load_average.one_minute,
    	    load_average.five_minutes,
    	    load_average.fifteen_minutes
	);

        printf("CPU Usage:       %6.2f%%\n",
               cpu_usage);

        printf("Memory Usage:    %6.2f%%\n",
               memory_info.usage_percent);

        printf("Disk Usage:      %6.2f%%\n",
               disk_info.usage_percent);

        printf("Disk Total:      %6.2f GiB\n",
               disk_info.total_bytes / BYTES_PER_GIB);

        printf("Disk Used:       %6.2f GiB\n",
               disk_info.used_bytes / BYTES_PER_GIB);

        printf("Disk Available:  %6.2f GiB\n",
               disk_info.available_bytes / BYTES_PER_GIB);

        printf("--------------------------------\n");

        /*
         * 为下一轮 CPU 计算保存当前数据。
         */
        previous_cpu = current_cpu;
    }

    printf("\nEdgeSentinel stopped safely.\n");

    return 0;
}
