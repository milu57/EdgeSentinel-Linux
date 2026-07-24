#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "network.h"
#include "system_monitor.h"
#include "system_status.h"

/*
 * 1 GiB = 1024 × 1024 × 1024 字节。
 */
#define BYTES_PER_GIB 1073741824.0

/*
 * 1 MiB = 1024 × 1024 字节。
 */
#define BYTES_PER_MIB 1048576.0

/*
 * 控制程序主循环。
 *
 * volatile：
 *     表示该变量可能被信号处理函数异步修改。
 *
 * sig_atomic_t：
 *     表示对该变量的读写可以安全地由信号处理函数完成。
 */
static volatile sig_atomic_t keep_running = 1;

/*
 * Ctrl+C 对应 SIGINT 信号。
 *
 * 收到信号后不直接结束程序，
 * 而是把循环控制变量设置为0。
 */
static void handle_sigint(int signal_number)
{
    (void)signal_number;
    keep_running = 0;
}

/*
 * 计算两个单调时钟时间点之间经过的秒数。
 */
static double calculate_elapsed_seconds(
    const struct timespec *start,
    const struct timespec *end
)
{
    double seconds;
    double nanoseconds;

    seconds = (double)(end->tv_sec - start->tv_sec);

    nanoseconds =
        (double)(end->tv_nsec - start->tv_nsec)
        / 1000000000.0;

    return seconds + nanoseconds;
}

int main(void)
{
    struct sigaction action;

    /*
     * CPU 前后两次采样。
     */
    CpuTimes previous_cpu;
    CpuTimes current_cpu;

    /*
     * 网络前后两次采样。
     */
    NetworkInfo previous_network;
    NetworkInfo current_network;

    /*
     * 网络实时速度及其显示形式。
     */
    NetworkSpeed network_speed;
    NetworkSpeedDisplay download_display;
    NetworkSpeedDisplay upload_display;

    /*
     * 网络两次采样对应的单调时钟时间。
     */
    struct timespec previous_network_time;
    struct timespec current_network_time;

    MemoryInfo memory_info;
    DiskInfo disk_info;
    SystemUptime system_uptime;
    LoadAverage load_average;
    CurrentTime current_time;

    unsigned long long uptime_seconds;

    double cpu_usage;
    double network_elapsed_seconds;

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
     * CPU 使用率需要两次采样。
     *
     * 启动时先读取第一次累计 CPU 时间，
     * 作为后续计算的 previous_cpu。
     */
    if (read_cpu_times(&previous_cpu) != 0)
    {
        fprintf(stderr, "Failed to read initial CPU times\n");
        return 1;
    }

    /*
     * 实时网速也需要两次采样。
     *
     * 启动时先读取第一次累计网络流量，
     * 作为后续计算的 previous_network。
     */
    if (read_network_info(&previous_network) != 0)
    {
        fprintf(stderr, "Failed to read initial network information\n");
        return 1;
    }

    /*
     * 记录第一次网络采样的时间。
     *
     * CLOCK_MONOTONIC 不会受到系统日期修改的影响，
     * 适合计算两个采样点之间实际经过的时间。
     */
    if (clock_gettime(
            CLOCK_MONOTONIC,
            &previous_network_time
        ) != 0)
    {
        perror("clock_gettime");
        return 1;
    }

    printf("EdgeSentinel system monitor started.\n");
    printf("Monitoring disk mount point: /\n");
    printf("Monitoring all non-loopback network interfaces.\n");
    printf("Press Ctrl+C to stop.\n\n");

    while (keep_running)
    {
        /*
         * 每隔约1秒采样一次。
         */
        sleep(1);

        /*
         * Ctrl+C 可能使 sleep 提前结束。
         */
        if (!keep_running)
        {
            break;
        }

        /*
         * 读取当前 CPU 累计时间。
         */
        if (read_cpu_times(&current_cpu) != 0)
        {
            fprintf(stderr, "Failed to read current CPU times\n");
            return 1;
        }

        /*
         * 根据前后两次 CPU 累计时间计算使用率。
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
         * 读取当前累计网络流量。
         */
        if (read_network_info(&current_network) != 0)
        {
            fprintf(stderr, "Failed to read current network information\n");
            return 1;
        }

        /*
         * 记录当前网络采样时间。
         */
        if (clock_gettime(
                CLOCK_MONOTONIC,
                &current_network_time
            ) != 0)
        {
            perror("clock_gettime");
            return 1;
        }

        /*
         * 计算前后两次网络采样之间的真实时间差。
         */
        network_elapsed_seconds =
            calculate_elapsed_seconds(
                &previous_network_time,
                &current_network_time
            );

        /*
         * 根据累计流量差值和时间差计算实时网速。
         */
        if (calculate_network_speed(
                &previous_network,
                &current_network,
                network_elapsed_seconds,
                &network_speed
            ) != 0)
        {
            fprintf(stderr, "Failed to calculate network speed\n");
            return 1;
        }

        /*
         * 自动选择下载速度的显示单位。
         */
        if (convert_network_speed(
                network_speed.download_bytes_per_sec,
                &download_display
            ) != 0)
        {
            fprintf(stderr, "Failed to convert download speed\n");
            return 1;
        }

        /*
         * 自动选择上传速度的显示单位。
         */
        if (convert_network_speed(
                network_speed.upload_bytes_per_sec,
                &upload_display
            ) != 0)
        {
            fprintf(stderr, "Failed to convert upload speed\n");
            return 1;
        }

        /*
         * 读取内存信息。
         */
        if (get_memory_info(&memory_info) != 0)
        {
            fprintf(stderr, "Failed to read memory information\n");
            return 1;
        }

        /*
         * 读取根文件系统 / 的磁盘信息。
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
         * 读取1、5、15分钟系统负载。
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
            "Updated:          %04d-%02d-%02d %02d:%02d:%02d\n",
            current_time.year,
            current_time.month,
            current_time.day,
            current_time.hour,
            current_time.minute,
            current_time.second
        );

        printf(
            "System Uptime:    %llu days %u hours "
            "%u minutes %u seconds\n",
            system_uptime.days,
            system_uptime.hours,
            system_uptime.minutes,
            system_uptime.seconds
        );

        printf(
            "Load Average:     %.2f  %.2f  %.2f\n",
            load_average.one_minute,
            load_average.five_minutes,
            load_average.fifteen_minutes
        );

        printf(
            "CPU Usage:        %6.2f%%\n",
            cpu_usage
        );

        printf(
            "Memory Usage:     %6.2f%%\n",
            memory_info.usage_percent
        );

        printf(
            "Disk Usage:       %6.2f%%\n",
            disk_info.usage_percent
        );

        printf(
            "Disk Total:       %6.2f GiB\n",
            disk_info.total_bytes / BYTES_PER_GIB
        );

        printf(
            "Disk Used:        %6.2f GiB\n",
            disk_info.used_bytes / BYTES_PER_GIB
        );

        printf(
            "Disk Available:   %6.2f GiB\n",
            disk_info.available_bytes / BYTES_PER_GIB
        );

        /*
         * rx 表示 Receive，即累计接收流量。
         * 接收流量对应通常所说的下载流量。
         */
        printf(
            "Network RX Total: %8.2f MiB\n",
            current_network.rx_bytes / BYTES_PER_MIB
        );

        /*
         * tx 表示 Transmit，即累计发送流量。
         * 发送流量对应通常所说的上传流量。
         */
        printf(
            "Network TX Total: %8.2f MiB\n",
            current_network.tx_bytes / BYTES_PER_MIB
        );

        printf(
            "Download Speed:   %8.2f %-4s\n",
            download_display.value,
            download_display.unit
        );

        printf(
            "Upload Speed:     %8.2f %-4s\n",
            upload_display.value,
            upload_display.unit
        );

        printf("---------------------------------\n");

        /*
         * 当前 CPU 采样成为下一轮的前一次采样。
         */
        previous_cpu = current_cpu;

        /*
         * 当前网络流量和时间成为下一轮的前一次采样。
         */
        previous_network = current_network;
        previous_network_time = current_network_time;
    }

    printf("\nEdgeSentinel stopped safely.\n");

    return 0;
}
