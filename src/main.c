#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "alert.h"
#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "logger.h"
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
 * 日志目录和日志文件路径。
 *
 * 这里使用相对路径，因此程序从项目根目录运行时，
 * 日志会保存到：
 *
 * EdgeSentinel-Linux/logs/edgesentinel.log
 */


#define LOG_INTERVAL_SAMPLES 10U
/*
 * 控制程序主循环。
 *
 * volatile：
 *     表示该变量可能被信号处理函数异步修改。
 *
 * sig_atomic_t：
 *     表示该变量适合在信号处理函数中读写。
 */
static volatile sig_atomic_t keep_running = 1;

/*
 * Ctrl+C 对应 SIGINT 信号。
 *
 * 收到 Ctrl+C 后，不直接强制结束程序，
 * 而是把 keep_running 设置为 0。
 */
static void handle_sigint(int signal_number)
{
    /*
     * 当前函数不需要使用信号编号。
     *
     * 这一行用于明确表示：
     * 我们知道有这个参数，但这里故意不使用。
     */
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

    /*
     * 计算整秒部分的差值。
     */
    seconds = (double)(end->tv_sec - start->tv_sec);

    /*
     * 计算纳秒部分的差值，并转换成秒。
     */
    nanoseconds =
        (double)(end->tv_nsec - start->tv_nsec)
        / 1000000000.0;

    return seconds + nanoseconds;
}

int main(void)
{

    AppConfig config;

    /*
     * 第一步：先写入默认值。
     */
    config_set_defaults(&config);

        if (
        logger_init(
            config.log_file,
            config.log_max_size
        ) != 0
    )
    {
        fprintf(
            stderr,
            "Failed to initialize logger: %s\n",
            config.log_file
        );

        return 1;
    }
    /*
     * 第二步：读取配置文件。
     * 配置文件中存在的参数会覆盖默认值。
     */
    if (config_load("config/edgesentinel.conf", &config) != 0) {
        fprintf(
            stderr,
            "Warning: failed to load or parse configuration file, "
            "using default configuration.\n"
        );
    }

    if (config_validate(&config) != 0)
    {
        fprintf(
            stderr,
            "Warning: invalid configuration, "
            "using default configuration.\n"
        );

        config_set_defaults(&config);
    }

    /*
     * 暂时打印最终生效的配置，验证读取是否成功。
     */
    config_print(&config);

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
     * 网络实时速度。
     */
    NetworkSpeed network_speed;

    /*
     * 转换单位后的下载速度和上传速度。
     */
    NetworkSpeedDisplay download_display;
    NetworkSpeedDisplay upload_display;

    /*
     * 网络两次采样对应的单调时钟时间。
     */
    struct timespec previous_network_time;
    struct timespec current_network_time;

    /*
     * 系统监控数据。
     */
    MemoryInfo memory_info;
    DiskInfo disk_info;
    SystemUptime system_uptime;
    LoadAverage load_average;
    CurrentTime current_time;

    /*
     * 各项资源的告警等级。
     *
     * 每个变量在某一时刻只保存一个状态：
     *
     * ALERT_NORMAL
     * ALERT_WARNING
     * ALERT_CRITICAL
     */
    AlertLevel cpu_level;
    AlertLevel memory_level;
    AlertLevel disk_level;
    AlertLevel system_level;

	/*
	 * 保存上一次采样得到的系统状态，
	 * 用于判断状态是否发生变化。
	 */
	AlertLevel previous_system_level;
	
	/*
	 * 标记 previous_system_level 是否已经获得有效值。
	 *
	 * 0：还没有上一次状态
	 * 1：已经保存了上一次状态
	 */
	int system_level_initialized = 0;

    /*
     * 系统启动后经过的总秒数。
     */
    unsigned long long uptime_seconds;

    /*
     * CPU 使用率。
     */
    double cpu_usage;

    /*
     * 两次网络采样之间经过的真实时间。
     */
    double network_elapsed_seconds;

    char log_message[256];
    int log_message_length;

    /*
     * 记录已经完成了多少次监控采样。 
     */
    unsigned int log_sample_counter = 0;

    /*
     * 配置 Ctrl+C 信号处理。
     */
    action.sa_handler = handle_sigint;

    /*
     * 清空信号屏蔽集合。
     */
    sigemptyset(&action.sa_mask);

    /*
     * 当前不使用额外的 sigaction 标志。
     */
    action.sa_flags = 0;

    /*
     * 将 handle_sigint 注册为 SIGINT 的处理函数。
     */
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    /*
     * CPU 使用率需要比较前后两次累计 CPU 时间。
     *
     * 程序启动时先读取第一次数据，
     * 保存到 previous_cpu。
     */
    if (read_cpu_times(&previous_cpu) != 0)
    {
        fprintf(
            stderr,
            "Failed to read initial CPU times\n"
        );

        return 1;
    }

    /*
     * 实时网速也需要比较前后两次累计流量。
     *
     * 程序启动时先读取第一次网络流量。
     */
    if (read_network_info(&previous_network) != 0)
    {
        fprintf(
            stderr,
            "Failed to read initial network information\n"
        );

        return 1;
    }

    /*
     * 记录第一次网络采样时间。
     *
     * CLOCK_MONOTONIC 是单调时钟，
     * 不会因为修改系统日期而突然向前或向后跳动。
     */
    if (clock_gettime(
            CLOCK_MONOTONIC,
            &previous_network_time
        ) != 0)
    {
        perror("clock_gettime");
        return 1;
    }

    /*
     * 创建日志目录。
     *
     * 如果 logs 目录已经存在，
     * logger_init() 仍然会返回成功。
     */
    if (
        logger_init(
            config.log_file,
            config.log_max_size
        ) != 0
    )
    {
        fprintf(
            stderr,
            "Failed to initialize logger: %s\n",
            config.log_file
        );
    
        return 1;
    }

    /*
     * 写入程序启动日志。
     */
    if (logger_write(
           config.log_file,
            "INFO",
            "EdgeSentinel started"
        ) != 0)
    {
        fprintf(stderr, "Failed to write startup log\n");
        return 1;
    }

    printf("EdgeSentinel system monitor started.\n");
    printf("Monitoring disk mount point: /\n");
    printf(
        "Monitoring all non-loopback network interfaces.\n"
    );
    printf("Press Ctrl+C to stop.\n\n");

    /*
     * 只要 keep_running 不等于 0，
     * 程序就持续进行监控。
     */
    while (keep_running)
    {
        /*
         * 每隔约 1 秒采样一次。
         */
        sleep(config.monitor_interval);

        /*
         * Ctrl+C 可能使 sleep 提前结束。
         *
         * 因此 sleep 返回后再次检查 keep_running。
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
            fprintf(
                stderr,
                "Failed to read current CPU times\n"
            );

            return 1;
        }

        /*
         * 根据前后两次 CPU 累计时间计算使用率。
         */
        cpu_usage = calculate_cpu_usage(
            &previous_cpu,
            &current_cpu
        );

        /*
         * 小于 0 表示 CPU 使用率计算失败。
         */
        if (cpu_usage < 0.0)
        {
            fprintf(
                stderr,
                "Failed to calculate CPU usage\n"
            );

            return 1;
        }

        cpu_level = alert_evaluate_percentage(
	    cpu_usage,
	    config.cpu_warning_threshold,
	    config.cpu_critical_threshold
	);

        /*
         * 读取当前累计网络流量。
         */
        if (read_network_info(&current_network) != 0)
        {
            fprintf(
                stderr,
                "Failed to read current network information\n"
            );

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
         * 计算前后两次网络采样之间经过的真实时间。
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
            fprintf(
                stderr,
                "Failed to calculate network speed\n"
            );

            return 1;
        }

        /*
         * 自动选择下载速度显示单位。
         *
         * 例如：
         *
         * B/s
         * KiB/s
         * MiB/s
         */
        if (convert_network_speed(
                network_speed.download_bytes_per_sec,
                &download_display
            ) != 0)
        {
            fprintf(
                stderr,
                "Failed to convert download speed\n"
            );

            return 1;
        }


        /*
         * 自动选择上传速度显示单位。
         */
        if (convert_network_speed(
                network_speed.upload_bytes_per_sec,
                &upload_display
            ) != 0)
        {
            fprintf(
                stderr,
                "Failed to convert upload speed\n"
            );

            return 1;
        }

        /*
         * 读取内存信息。
         */
        if (get_memory_info(&memory_info) != 0)
        {
            fprintf(
                stderr,
                "Failed to read memory information\n"
            );

            return 1;
        }

        memory_level = alert_evaluate_percentage(
	    memory_info.usage_percent,
	    config.memory_warning_threshold,
	    config.memory_critical_threshold
	);

        /*
         * 读取根文件系统 / 的磁盘信息。
         */
        if (get_disk_info("/", &disk_info) != 0)
        {
            fprintf(
                stderr,
                "Failed to read disk information\n"
            );

            return 1;
        }

        disk_level = alert_evaluate_percentage(
	    disk_info.usage_percent,
	    config.disk_warning_threshold,
	    config.disk_critical_threshold
	);

        /*
         * 计算整个系统的统一告警状态。
         *
         * 第一次：
         * 比较 CPU 和内存，得到两者中更严重的状态。
         */
        system_level = alert_get_higher_level(
            cpu_level,
            memory_level
        );

        /*
         * 第二次：
         * 将上一步结果与磁盘状态比较。
         *
         * 最终 system_level 就是三个资源中最严重的状态。
         */
        system_level = alert_get_higher_level(
            system_level,
            disk_level
        );

	/*
	 * 第一次采样时还没有上一次状态，
	 * 因此只保存当前状态，不记录“状态变化”。
	 */
	if (!system_level_initialized)
	{
	    previous_system_level = system_level;
	    system_level_initialized = 1;
	}
	/*
	 * 从第二次采样开始，比较当前状态和上一次状态。
	 */
	else if (system_level != previous_system_level)
	{
	    /*
	     * 生成状态变化日志。
	     *
	     * 例如：
	     * System status changed: NORMAL -> WARNING
	     */
	    log_message_length = snprintf(
	        log_message,
	        sizeof(log_message),
	        "System status changed: %s -> %s "
	        "CPU=%.2f%% Memory=%.2f%% Disk=%.2f%%",
	        alert_level_to_string(previous_system_level),
	        alert_level_to_string(system_level),
	        cpu_usage,
	        memory_info.usage_percent,
	        disk_info.usage_percent
	    );
	
	    /*
	     * snprintf() 返回负数表示格式化失败。
	     *
	     * 返回值大于或等于数组容量，
	     * 表示字符串过长并被截断。
	     */
	    if (
	        log_message_length < 0 ||
	        (size_t)log_message_length >= sizeof(log_message)
	    )
	    {
	        fprintf(
	            stderr,
	            "Failed to format status change log\n"
	        );
	
	        return 1;
	    }
	
	    /*
	     * 状态变化时立即写日志。
	     *
	     * 当前状态同时作为日志等级。
	     */
	    if (
	        logger_write(
	            config.log_file,
	            alert_level_to_string(system_level),
	            log_message
	        ) != 0
	    )
	    {
	        fprintf(
	            stderr,
	            "Failed to write status change log\n"
	        );
	
	        return 1;
	    }
	
	    /*
	     * 当前状态成为下一轮的上一次状态。
	     */
	    previous_system_level = system_level;
	}

	/*
	 * 当前轮次完成一次采样，因此计数器加 1。
	 */
	log_sample_counter++;

	/*
	 * 只有累计达到规定次数时，
	 * 才将本轮监控结果写入日志。
	 */
	if (log_sample_counter >= LOG_INTERVAL_SAMPLES)
	{
	    /*
	     * 将 CPU、内存和磁盘数据组合成一条字符串。
	     */
	    if (
	        snprintf(
	            log_message,
	            sizeof(log_message),
	            "CPU=%.2f%% Memory=%.2f%% Disk=%.2f%%",
	            cpu_usage,
	            memory_info.usage_percent,
	            disk_info.usage_percent
	        ) < 0
	    )
	    {
	        fprintf(
	            stderr,	
	            "Failed to format monitoring log\n"
	        );

	        return 1;
	    }

	    /*
	     * 将格式化后的监控结果追加到日志文件。
	     */
	    if (
	        logger_write(
	            config.log_file,
	            alert_level_to_string(system_level),
	            log_message
	        ) != 0
	    )
	    {
	        fprintf(
	            stderr,
	            "Failed to write monitoring log\n"
	        );

	        return 1;
	    }

	    /*
	     * 本次日志已经写入，
	     * 将计数器清零并重新开始统计。
	     */
	    log_sample_counter = 0;
	}

        /*
         * 读取系统启动后经过的总秒数。
         */
        if (get_system_uptime(&uptime_seconds) != 0)
        {
            fprintf(
                stderr,
                "Failed to read system uptime\n"
            );

            return 1;
        }

        /*
         * 将总秒数转换成：
         *
         * 天
         * 小时
         * 分钟
         * 秒
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
            fprintf(
                stderr,
                "Failed to read load average\n"
            );

            return 1;
        }

        /*
         * 获取当前系统本地时间。
         */
        if (get_current_time(&current_time) != 0)
        {
            fprintf(
                stderr,
                "Failed to get current time\n"
            );

            return 1;
        }

        /*
         * 输出当前时间。
         */
        printf(
            "Updated:          "
            "%04d-%02d-%02d %02d:%02d:%02d\n",
            current_time.year,
            current_time.month,
            current_time.day,
            current_time.hour,
            current_time.minute,
            current_time.second
        );

        /*
         * 输出系统运行时间。
         */
        printf(
            "System Uptime:    "
            "%llu days %u hours %u minutes %u seconds\n",
            system_uptime.days,
            system_uptime.hours,
            system_uptime.minutes,
            system_uptime.seconds
        );

        /*
         * 输出 1、5、15 分钟平均负载。
         */
        printf(
            "Load Average:     %.2f  %.2f  %.2f\n",
            load_average.one_minute,
            load_average.five_minutes,
            load_average.fifteen_minutes
        );

        /*
         * 输出 CPU 使用率及其告警状态。
         */
        printf(
            "CPU Usage:        %6.2f%% [%s]\n",
            cpu_usage,
            alert_level_to_string(cpu_level)
        );

        /*
         * 输出内存使用率及其告警状态。
         */
        printf(
            "Memory Usage:     %6.2f%% [%s]\n",
            memory_info.usage_percent,
            alert_level_to_string(memory_level)
        );

        /*
         * 输出磁盘使用率及其告警状态。
         */
        printf(
            "Disk Usage:       %6.2f%% [%s]\n",
            disk_info.usage_percent,
            alert_level_to_string(disk_level)
        );

        /*
         * 输出整个系统的统一告警状态。
         */
        printf(
            "System Status:           [%s]\n",
            alert_level_to_string(system_level)
        );

        /*
         * 输出磁盘总容量。
         */
        printf(
            "Disk Total:       %6.2f GiB\n",
            disk_info.total_bytes / BYTES_PER_GIB
        );

        /*
         * 输出磁盘已使用容量。
         */
        printf(
            "Disk Used:        %6.2f GiB\n",
            disk_info.used_bytes / BYTES_PER_GIB
        );

        /*
         * 输出磁盘可用容量。
         */
        printf(
            "Disk Available:   %6.2f GiB\n",
            disk_info.available_bytes / BYTES_PER_GIB
        );

        /*
         * RX 表示 Receive，即累计接收流量。
         *
         * 接收流量通常对应下载流量。
         */
        printf(
            "Network RX Total: %8.2f MiB\n",
            current_network.rx_bytes / BYTES_PER_MIB
        );

        /*
         * TX 表示 Transmit，即累计发送流量。
         *
         * 发送流量通常对应上传流量。
         */
        printf(
            "Network TX Total: %8.2f MiB\n",
            current_network.tx_bytes / BYTES_PER_MIB
        );

        /*
         * 输出实时下载速度。
         */
        printf(
            "Download Speed:   %8.2f %-4s\n",
            download_display.value,
            download_display.unit
        );

        /*
         * 输出实时上传速度。
         */
        printf(
            "Upload Speed:     %8.2f %-4s\n",
            upload_display.value,
            upload_display.unit
        );

        /*
         * 分隔不同采样周期的输出。
         */
        printf("---------------------------------\n");

        /*
         * 当前 CPU 采样成为下一轮的前一次采样。
         */
        previous_cpu = current_cpu;

        /*
         * 当前网络累计流量成为下一轮的前一次采样。
         */
        previous_network = current_network;

        /*
         * 当前网络采样时间成为下一轮的前一次时间。
         */
        previous_network_time = current_network_time;
    }

    if (logger_write(
            config.log_file,
            "INFO",
            "EdgeSentinel stopped safely"
        ) != 0)
    {
        fprintf(stderr, "Failed to write shutdown log\n");
    }


    /*
     * 离开循环后进行安全退出提示。
     */
    printf("\nEdgeSentinel stopped safely.\n");

    return 0;
}
