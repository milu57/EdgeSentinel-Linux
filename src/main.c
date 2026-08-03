#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "alert.h"
#include "alert_event.h"
#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "logger.h"
#include "network.h"
#include "notifier.h"
#include "output.h"
#include "process_monitor.h"
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
 * 默认配置文件路径。
 *
 * 用户没有通过 -c 指定配置文件时，
 * 程序使用该路径。
 */
#define DEFAULT_CONFIG_FILE "config/edgesentinel.conf"

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
 * 标记是否收到配置重新加载请求。
 *
 * 0：没有收到 SIGHUP；
 * 1：已经收到 SIGHUP，需要重新读取配置文件。
 */
static volatile sig_atomic_t reload_requested = 0;

/*
 * 处理程序停止信号。
 *
 * SIGINT：
 *     用户在终端按 Ctrl+C 时产生。
 *
 * SIGTERM：
 *     systemd 停止服务时默认发送。
 *
 * 收到停止信号后不直接执行复杂清理，
 * 只把 keep_running 设置为 0。
 * 主循环随后自然结束，并执行退出前的清理工作。
 */
static void handle_stop_signal(int signal_number)
{
    /*
     * 当前不需要区分具体收到的是
     * SIGINT 还是 SIGTERM。
     */
    (void)signal_number;

    keep_running = 0;
}

/*
 * 处理配置重新加载信号。
 *
 * 收到 SIGHUP 时，不停止程序，
 * 只设置重新加载标志。
 *
 * 配置文件的实际读取工作由主循环完成。
 */
static void handle_reload_signal(int signal_number)
{
    (void)signal_number;

    reload_requested = 1;
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

/*
 * 在告警等级发生变化时发送通知。
 *
 * 通知关闭时直接返回；
 * 通知失败只输出错误，不终止系统监控。
 */
static void send_alert_change_notification(
    const AppConfig *config,
    AlertMetric metric,
    const char *target,
    AlertLevel previous_level,
    AlertLevel current_level,
    double current_value,
    const char *unit
)
{
    AlertEvent event;

    if (
        config == NULL ||
        target == NULL ||
        unit == NULL
    ) {
        return;
    }

    /*
     * 只有显式启用通知时才发送。
     */
    if (config->notification_enabled != 1) {
        return;
    }

    /*
     * 告警等级没有变化时不重复发送。
     */
    if (previous_level == current_level) {
        return;
    }

    if (
        alert_event_init(
            &event,
            metric,
            target,
            previous_level,
            current_level,
            current_value,
            unit
        ) != 0
    ) {
        fprintf(
            stderr,
            "Failed to create alert notification event: "
            "metric=%s target=%s\n",
            alert_metric_to_string(metric),
            target
        );

        return;
    }

    /*
     * 通知属于附加功能。
     *
     * 即使发送失败，核心监控程序也继续运行。
     */
    if (
        notifier_send(
            &event,
            config->notification_command
        ) != 0
    ) {
        fprintf(
            stderr,
            "Failed to send alert notification: "
            "metric=%s target=%s\n",
            alert_metric_to_string(metric),
            target
        );
    }
}

/*
 * 完成一个目标进程的一轮监控。
 *
 * 返回：
 *     0  成功
 *    -1  发生错误
 */
static int monitor_process_once(
    MonitoredProcess *process,
    const AppConfig *config,
    FILE *status_stream
)
{

    ProcessCpuTimes current_process_cpu_times;
    struct timespec current_process_cpu_time;
    double process_elapsed_seconds;
    char log_message[256];
    int log_message_length;
    char notification_target[
        ALERT_EVENT_TARGET_LENGTH
    ];

    int notification_target_length;

    if (
        process == NULL ||
        config == NULL ||
        status_stream == NULL
    )
    {
        fprintf(
            stderr,
            "Invalid monitored process or configuration\n"
        );

        return -1;
    }

    /*
     * 使用进程名称监控时，如果当前没有有效 PID，
     * 就重新扫描 /proc 查找目标进程。
     */
    if (
        process->target_name[0] != '\0' &&
        process->current_pid == 0
    )
    {
        if (
            find_process_by_name(
                process->target_name,
                &process->current_pid
            ) == 0
        )
        {
            fprintf(
                status_stream,
                "Target process found: %s (PID: %d)\n",
                process->target_name,
                process->current_pid
            );

            /*
             * 新找到的 PID 代表新的进程实例。
             * 旧进程的采样数据不能继续使用。
             */
            monitored_process_reset_runtime_state(
                process
            );
        }
    }

    /*
     * 读取目标进程的信息。
     */
    if (
        process->current_pid <= 0 ||
        read_process_info(
            process->current_pid,
            &process->info
        ) != 0
    )
    {
        process->available = 0;
    }
    else
    {
        process->available = 1;
        /*
         * 为通知生成能够区分具体进程的目标名称。
         *
         * 示例：
         *     sleep PID=1234
         */
        notification_target_length = snprintf(
            notification_target,
            sizeof(notification_target),
            "%s PID=%d",
            process->info.name[0] != '\0'
                ? process->info.name
                : "process",
            process->current_pid
        );

        if (
            notification_target_length < 0 ||
            (size_t)notification_target_length >=
                sizeof(notification_target)
        ) {
            fprintf(
                stderr,
                "Failed to format process notification target\n"
            );

            return -1;
        }

        /*
         * VmRSS 的单位是 kB。
         * 1024 kB = 1 MiB。
         */
        process->memory_mib =
            (double)process->info.resident_memory_kb / 1024.0;

        /*
         * 计算进程内存告警等级。
         */
        process->memory_level =
            alert_evaluate_percentage(
                process->memory_mib,
                config->process_memory_warning_threshold_mib,
                config->process_memory_critical_threshold_mib
            );

        /*
         * 第一次取得告警等级时只保存基准。
         */
        if (!process->memory_level_initialized)
        {
            process->previous_memory_level =
                process->memory_level;

            process->memory_level_initialized = 1;
        }
        /*
         * 从第二次开始判断内存告警等级是否变化。
         */
        else if (
            process->memory_level !=
            process->previous_memory_level
        )
        {
            log_message_length = snprintf(
                log_message,
                sizeof(log_message),
                "Process memory status changed: "
                "%s -> %s PID=%d Memory=%.2f MiB",
                alert_level_to_string(
                    process->previous_memory_level
                ),
                alert_level_to_string(
                    process->memory_level
                ),
                process->current_pid,
                process->memory_mib
            );

            if (
                log_message_length < 0 ||
                (size_t)log_message_length >=
                    sizeof(log_message)
            )
            {
                fprintf(
                    stderr,
                    "Failed to format process memory status log\n"
                );

                return -1;
            }

            if (
                logger_write(
                    config->log_file,
                    alert_level_to_string(
                        process->memory_level
                    ),
                    log_message
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Failed to write process memory status log\n"
                );

                return -1;
            }

            send_alert_change_notification(
                config,
                ALERT_METRIC_PROCESS_MEMORY,
                notification_target,
                process->previous_memory_level,
                process->memory_level,
                process->memory_mib,
                "MiB"
            );

            process->previous_memory_level =
                process->memory_level;
        }
    }

    /*
     * 每轮开始时先认为没有有效 CPU 结果。
     */
    process->cpu_usage_valid = 0;

    /*
     * 只有目标进程可用时才读取 CPU 累计时间。
     */
    if (process->available)
    {
        if (
            read_process_cpu_times(
                process->current_pid,
                &current_process_cpu_times
            ) != 0
        )
        {
            /*
             * 进程可能在读取期间退出。
             */
            monitored_process_reset_cpu_sampling(
                process
            );

            process->memory_level_initialized = 0;
        }
        else
        {
            if (
                clock_gettime(
                    CLOCK_MONOTONIC,
                    &current_process_cpu_time
                ) != 0
            )
            {
                perror("clock_gettime");
                return -1;
            }

            /*
             * 第一次采样只能建立基准。
             */
            if (!process->cpu_sample_initialized)
            {
                process->previous_cpu_times =
                    current_process_cpu_times;

                process->previous_cpu_sample_time =
                    current_process_cpu_time;

                process->cpu_sample_initialized = 1;
            }
            else
            {
                process_elapsed_seconds =
                    calculate_elapsed_seconds(
                        &process->previous_cpu_sample_time,
                        &current_process_cpu_time
                    );

                process->cpu_usage =
                    calculate_process_cpu_usage(
                        &process->previous_cpu_times,
                        &current_process_cpu_times,
                        process_elapsed_seconds
                    );

                if (process->cpu_usage >= 0.0)
                {
                    process->cpu_usage_valid = 1;

                    process->cpu_level =
                        alert_evaluate_percentage(
                            process->cpu_usage,
                            config->process_cpu_warning_threshold,
                            config->process_cpu_critical_threshold
                        );

                    /*
                     * 第一次获得 CPU 告警等级时只保存基准。
                     */
                    if (!process->cpu_level_initialized)
                    {
                        process->previous_cpu_level =
                            process->cpu_level;

                        process->cpu_level_initialized = 1;
                    }
                    /*
                     * 判断 CPU 告警等级是否变化。
                     */
                    else if (
                        process->cpu_level !=
                        process->previous_cpu_level
                    )
                    {
                        log_message_length = snprintf(
                            log_message,
                            sizeof(log_message),
                            "Process CPU status changed: "
                            "%s -> %s "
                            "PID=%d CPU=%.2f%%",
                            alert_level_to_string(
                                process->previous_cpu_level
                            ),
                            alert_level_to_string(
                                process->cpu_level
                            ),
                            process->current_pid,
                            process->cpu_usage
                        );

                        if (
                            log_message_length < 0 ||
                            (size_t)log_message_length >=
                                sizeof(log_message)
                        )
                        {
                            fprintf(
                                stderr,
                                "Failed to format process CPU status log\n"
                            );

                            return -1;
                        }

                        if (
                            logger_write(
                                config->log_file,
                                alert_level_to_string(
                                    process->cpu_level
                                ),
                                log_message
                            ) != 0
                        )
                        {
                            fprintf(
                                stderr,
                                "Failed to write process CPU status log\n"
                            );

                            return -1;
                        }

                        send_alert_change_notification(
                            config,
                            ALERT_METRIC_PROCESS_CPU,
                            notification_target,
                            process->previous_cpu_level,
                            process->cpu_level,
                            process->cpu_usage,
                            "%"
                        );

                        process->previous_cpu_level =
                            process->cpu_level;
                    }
                }

                /*
                 * 当前采样成为下一轮的比较基准。
                 */
                process->previous_cpu_times =
                    current_process_cpu_times;

                process->previous_cpu_sample_time =
                    current_process_cpu_time;
            }
        }
    }
    else
    {
        /*
         * 进程不可用时，旧 CPU 基准不能继续使用。
         */
        monitored_process_reset_cpu_sampling(
            process
        );
    }

    /*
     * 第一次采样只保存进程可用状态。
     */
    if (!process->availability_initialized)
    {
        process->previous_available =
            process->available;

        process->availability_initialized = 1;
    }
    /*
     * 从第二次采样开始判断可用状态是否变化。
     */
    else if (
        process->available !=
        process->previous_available
    )
    {
        const char *process_log_level;

        if (process->available)
        {
            process_log_level = "INFO";

            log_message_length = snprintf(
                log_message,
                sizeof(log_message),
                "Monitored process recovered: "
                "PID=%d Name=%s State=%s Memory=%lu kB",
                process->info.pid,
                process->info.name,
                process->info.state,
                process->info.resident_memory_kb
            );
        }
        else
        {
            process_log_level = "WARNING";

            log_message_length = snprintf(
                log_message,
                sizeof(log_message),
                "Monitored process unavailable: PID=%d",
                process->current_pid
            );
        }

        if (
            log_message_length < 0 ||
            (size_t)log_message_length >=
                sizeof(log_message)
        )
        {
            fprintf(
                stderr,
                "Failed to format process status log\n"
            );

            return -1;
        }

        if (
            logger_write(
                config->log_file,
                process_log_level,
                log_message
            ) != 0
        )
        {
            fprintf(
                stderr,
                "Failed to write process status log\n"
            );

            return -1;
        }

        process->previous_available =
            process->available;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    /*
     * 当前准备读取的配置文件路径。
     *
     * 默认指向项目内的配置文件。
     */
    const char *config_file = DEFAULT_CONFIG_FILE;

    /*
     * 默认保持原来的文本输出方式。
     *
     * 用户可以通过：
     *
     *     --output json
     *
     * 切换为 JSON 输出。
     */
    OutputFormat output_format = OUTPUT_FORMAT_TEXT;

    /*
     * 普通文本模式下，状态提示输出到 stdout。
     *
     * JSON 模式下，stdout 必须只保留 JSON，
     * 因此启动信息和诊断信息改为输出到 stderr。
     */
    FILE *status_stream;

    /*
     * 遍历命令行参数时使用的数组下标。
     */
    int argument_index;

    AppConfig config;
    AppConfig reloaded_config;

    /*
     * 保存目标进程完整的长期监控状态。
     *
     * 当前阶段先让它接管目标名称和 PID，
     * 后面再逐步接管 CPU、内存和告警状态。
     */
    MonitoredProcess monitored_processes[MAX_MONITORED_PROCESSES];
    size_t monitored_process_count = 0;
    size_t process_index;
    MonitoredProcess *active_process;

    /*
     * 在 MonitoredProcess 初始化之前，
     * 临时保存第一次确定的目标 PID。
     */
    int initial_process_pid = 0;

    /*
     * 逐个解析命令行参数。
     *
     * 当前支持：
     *
     *     ./edgesentinel
     *
     *     ./edgesentinel -c config/edgesentinel.conf
     *
     *     ./edgesentinel --output json
     *
     *     ./edgesentinel
     *         -c config/edgesentinel.conf
     *         --output json
     *
     * -c 和 --output 的前后顺序不受限制。
     */
    for (
        argument_index = 1;
        argument_index < argc;
        argument_index++
    )
    {
        /*
         * 读取配置文件路径。
         */
        if (strcmp(argv[argument_index], "-c") == 0)
        {
            /*
             * -c 后面必须还有一个参数，
             * 作为配置文件路径。
             */
            if (argument_index + 1 >= argc)
            {
                fprintf(
                    stderr,
                    "Missing configuration file after -c.\n"
                );

                fprintf(
                    stderr,
                    "Usage: %s "
                    "[-c config_file] "
                    "[--output text|json]\n",
                    argv[0]
                );

                return 1;
            }

            /*
             * 跳到下一个参数并保存配置文件路径。
             */
            argument_index++;
            config_file = argv[argument_index];
        }
        /*
         * 读取输出格式。
         */
        else if (
            strcmp(
                argv[argument_index],
                "--output"
            ) == 0
        )
        {
            /*
             * --output 后面必须还有 text 或 json。
             */
            if (argument_index + 1 >= argc)
            {
                fprintf(
                    stderr,
                    "Missing format after --output.\n"
                );

                fprintf(
                    stderr,
                    "Usage: %s "
                    "[-c config_file] "
                    "[--output text|json]\n",
                    argv[0]
                );

                return 1;
            }

            /*
             * 跳到输出格式字符串。
             */
            argument_index++;

            if (
                output_parse_format(
                    argv[argument_index],
                    &output_format
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid output format: %s\n",
                    argv[argument_index]
                );

                fprintf(
                    stderr,
                    "Supported output formats: "
                    "text, json\n"
                );

                return 1;
            }
        }
        /*
         * 既不是 -c，也不是 --output，
         * 说明出现了程序无法识别的参数。
         */
        else
        {
            fprintf(
                stderr,
                "Unknown argument: %s\n",
                argv[argument_index]
            );

            fprintf(
                stderr,
                "Usage: %s "
                "[-c config_file] "
                "[--output text|json]\n",
                argv[0]
            );

            return 1;
        }
    }

    if (output_format == OUTPUT_FORMAT_JSON)
    {
        status_stream = stderr;
    }
    else
    {
        status_stream = stdout;
    }

    /*
     * 第一步：先写入默认值。
     */
    config_set_defaults(&config);

    /*
     * 第二步：读取配置文件。
     * 配置文件中存在的参数会覆盖默认值。
     */
    if (config_load(config_file, &config) != 0) {
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
     * 根据配置确定实际监控进程数量。
     */
    if (config.process_name_count > 0)
    {
        monitored_process_count =
            (size_t)config.process_name_count;
    }
    else
    {
        /*
         * 没有 process_names 时，
         * 继续兼容原来的单进程配置。
         */
        monitored_process_count = 1;
    }

    /*
     * 防止监控数量超过数组容量。
     */
    if (
        monitored_process_count >
        MAX_MONITORED_PROCESSES
    )
    {
        fprintf(
            stderr,
            "Too many monitored processes: %zu "
            "(maximum: %d)\n",
            monitored_process_count,
            MAX_MONITORED_PROCESSES
        );

        return 1;
    }

    /*
     * 依次初始化每个进程监控对象。
     */
    for (
        process_index = 0;
        process_index < monitored_process_count;
        process_index++
    )
    {
        const char *target_process_name;

        /*
         * 使用多进程配置时，
         * 每个数组元素取得自己的进程名称。
         */
        if (config.process_name_count > 0)
        {
            target_process_name =
                config.process_names[process_index];
        }
        else
        {
            target_process_name =
                config.process_name;
        }

        /*
         * 为当前目标进程查找初始 PID。
         */
        initial_process_pid = 0;

        if (target_process_name[0] != '\0')
        {
            if (
                find_process_by_name(
                    target_process_name,
                    &initial_process_pid
                ) != 0
            )
            {
                initial_process_pid = 0;

                fprintf(
                    status_stream,
                    "Target process is not running yet: %s\n",
                    target_process_name
                );
            }
        }
        else if (config.process_pid == 0)
        {
            initial_process_pid = (int)getpid();
        }
        else
        {
            initial_process_pid =
                (int)config.process_pid;
        }

        if (
            monitored_process_init(
                &monitored_processes[process_index],
                target_process_name,
                (int)config.process_pid,
                initial_process_pid
            ) != 0
        )
        {
            fprintf(
                stderr,
                "Failed to initialize monitored process %zu.\n",
                process_index
            );

            return 1;
        }
    }

    active_process = &monitored_processes[0];


    /*
     * 暂时打印最终生效的配置，验证读取是否成功。
     */

    fprintf(
        status_stream,
        "Configuration file: %s\n",
        config_file
    );

    /*
     * config_print() 当前固定输出到 stdout。
     * JSON 模式下暂时不调用，避免污染 JSON Lines。
     */
    if (output_format == OUTPUT_FORMAT_TEXT)
    {
        config_print(&config);
    }

    struct sigaction action;

    struct sigaction reload_action;

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
     * 集中保存当前一轮采样产生的全部监控结果。
     *
     * 文本输出和 JSON 输出读取同一份快照，
     * 不再分别读取或计算系统数据。
     */
    MonitorSnapshot snapshot = {0};

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

    /*
     * 标记网络采样基准是否刚刚因热加载而重置。
     *
     * 1：下一次采样只建立新基准，不计算网速；
     * 0：正常计算网速。
     */
    int network_baseline_reset = 0;

    char log_message[256];
    int log_message_length;

    /*
     * 记录已经完成了多少次监控采样。 
     */
    unsigned int log_sample_counter = 0;

    /*
     * 配置 Ctrl+C 信号处理。
     */
    action.sa_handler = handle_stop_signal;

    /*
     * 清空信号屏蔽集合。
     */
    sigemptyset(&action.sa_mask);

    /*
     * 当前不使用额外的 sigaction 标志。
     */
    action.sa_flags = 0;

    /*
     * 注册 SIGINT。
     *
     * 用户在终端按 Ctrl+C 时，
     * 调用 handle_stop_signal()。
     */
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction SIGINT");
        return 1;
    }

    /*
     * 注册 SIGTERM。
     *
     * systemd 执行 stop 操作时，
     * 默认向服务发送 SIGTERM。
     */
    if (sigaction(SIGTERM, &action, NULL) == -1)
    {
        perror("sigaction SIGTERM");
        return 1;
    }

    /*
     * 配置 SIGHUP 信号处理。
     */
    reload_action.sa_handler = handle_reload_signal;

    sigemptyset(&reload_action.sa_mask);

    reload_action.sa_flags = 0;

    /*
     * 注册 SIGHUP。
     *
     * 收到 SIGHUP 后调用 handle_reload_signal()，
     * 程序不会退出。
     */
    if (sigaction(SIGHUP, &reload_action, NULL) == -1)
    {
        perror("sigaction SIGHUP");
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
    if (
        read_network_info_filtered(
            &config,
            &previous_network
        ) != 0
    )
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

    fprintf(
        status_stream,
        "EdgeSentinel system monitor started.\n"
    );

    /*
     * 输出所有目标进程的启动状态。
     */
    for (
        process_index = 0;
        process_index < monitored_process_count;
        process_index++
    )
    {
        active_process =
            &monitored_processes[process_index];

        if (active_process->target_name[0] != '\0')
        {
            if (active_process->current_pid > 0)
            {
                fprintf(
                    status_stream,
                    "Monitoring process[%zu]: %s (PID: %d)\n",
                    process_index,
                    active_process->target_name,
                    active_process->current_pid
                );
            }
           else
            {
                fprintf(
                    status_stream,
                    "Waiting for process[%zu]: %s\n",
                    process_index,
                    active_process->target_name
                );
            }
        }
        else
        {
            fprintf(
                status_stream,
                "Monitoring process[%zu] PID: %d%s\n",
                process_index,
                active_process->current_pid,
                active_process->configured_pid == 0
                    ? " (self)"
                    : ""
            );
        }
    }

    fprintf(status_stream,"Monitoring disk mount point: /\n");
    if (config.network_interface_count == 0)
    {
        fprintf(
            status_stream,
            "Monitoring all non-loopback network interfaces.\n"
        );
    }
    else
    {
        unsigned int network_interface_index;

        fprintf(status_stream,"Monitoring selected network interfaces:");

        for (
            network_interface_index = 0;
            network_interface_index <
                config.network_interface_count;
            network_interface_index++
        )
        {
            fprintf(
                status_stream,
                "%s%s",
                network_interface_index == 0
                    ? " "
                    : ", ",
                config.network_interfaces[
                    network_interface_index
                ]
            );
        }

        fprintf(status_stream,".\n");
    }

    fprintf(status_stream,"Press Ctrl+C to stop.\n\n");

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
         * 收到 SIGHUP 后，重新读取配置文件。
         */
        if (reload_requested)
        {
            /*
             * 先清除标志。
             *
             * 如果重新加载期间再次收到 SIGHUP，
             * 信号处理函数会重新把它设置为 1。
             */
            reload_requested = 0;

            /*
             * 先给临时配置填入默认值。
             */
            config_set_defaults(&reloaded_config);

            /*
             * 把配置文件读取到临时结构体中，
             * 不直接修改当前正在使用的 config。
             */
            if (config_load(config_file, &reloaded_config) != 0)
            {
                fprintf(
                    stderr,
                    "Configuration reload failed, "
                    "keeping previous configuration.\n"
                );
            }
            else if (config_validate(&reloaded_config) != 0)
            {
                fprintf(
                    stderr,
                    "Reloaded configuration is invalid, "
                    "keeping previous configuration.\n"
                );
            }
            else if (
                logger_init(
                    reloaded_config.log_file,
                    reloaded_config.log_max_size
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Failed to apply reloaded logger configuration, "
                    "keeping previous configuration.\n"
                );
            }
            else if (
                read_network_info_filtered(
                    &reloaded_config,
                    &current_network
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Failed to read network baseline for "
                    "reloaded configuration, "
                    "keeping previous configuration.\n"
                );
            }
            else if (
                clock_gettime(
                    CLOCK_MONOTONIC,
                    &current_network_time
                ) != 0
            )
            {
                perror(
                    "clock_gettime for reloaded "
                    "network configuration"
                );
            }
            else
            {
                /*
                 * 应用新的正式配置。
                 */
                config = reloaded_config;

                /*
                 * 网络接口列表可能已经发生变化。
                 *
                 * 将按照新配置读取的累计流量和采样时间
                 * 设为新的速度计算基准，防止新旧接口的
                 * 累计值直接相减。
                 */
                previous_network =
                    current_network;

                previous_network_time =
                    current_network_time;

                network_baseline_reset = 1;

                /*
                 * 根据新配置重新确定实际监控进程数量。
                 */
                if (config.process_name_count > 0)
                {
                    monitored_process_count =
                        (size_t)config.process_name_count;
                }
                else
                {
                    /*
                     * 没有 process_names 时，
                     * 继续兼容原来的单进程配置。
                     */
                    monitored_process_count = 1;
                }

                /*
                 * 防止监控数量超过数组容量。
                 */
                if (
                    monitored_process_count >
                    MAX_MONITORED_PROCESSES
                )
                {
                    fprintf(
                        stderr,
                        "Too many monitored processes after reload: "
                        "%zu (maximum: %d)\n",
                        monitored_process_count,
                        MAX_MONITORED_PROCESSES
                    );

                    return 1;
                }

                /*
                 * 根据新配置重新初始化所有进程监控对象。
                 *
                 * 重新初始化后，旧 PID、CPU 采样和告警状态
                 * 不会继续影响新的目标进程。
                 */
                for (
                    process_index = 0;
                    process_index < monitored_process_count;
                    process_index++
                )
                {
                    const char *target_process_name;

                    if (config.process_name_count > 0)
                    {
                        target_process_name =
                            config.process_names[process_index];
                    }
                    else
                    {
                        target_process_name =
                            config.process_name;
                    }

                    initial_process_pid = 0;

                    /*
                     * 使用名称监控时，查找当前对应的 PID。
                     */
                    if (target_process_name[0] != '\0')
                    {
                        if (
                            find_process_by_name(
                                target_process_name,
                                &initial_process_pid
                            ) != 0
                        )
                        {
                            initial_process_pid = 0;
                        }
                    }
                    /*
                     * 没有名称且 PID 为 0 时，监控自身。
                     */
                    else if (config.process_pid == 0)
                    {
                        initial_process_pid =
                            (int)getpid();
                    }
                    /*
                     * 使用配置中的固定 PID。
                     */
                    else
                    {
                        initial_process_pid =
                            (int)config.process_pid;
                    }

                    if (
                        monitored_process_init(
                            &monitored_processes[process_index],
                            target_process_name,
                            (int)config.process_pid,
                            initial_process_pid
                        ) != 0
                    )
                    {
                        fprintf(
                            stderr,
                            "Failed to initialize reloaded "
                            "monitored process %zu.\n",
                            process_index
                        );

                        return 1;
                    }
                }

                /*
                 * 默认指向数组中的第一个进程。
                 */
                active_process =
                    &monitored_processes[0];

                fprintf(
                    status_stream,
                    "Configuration reloaded successfully.\n"
                );

                if (output_format == OUTPUT_FORMAT_TEXT)
                {
                    config_print(&config);
                }
                /*
                 * 输出热加载后的所有目标进程。
                 */
                for (
                    process_index = 0;
                    process_index < monitored_process_count;
                    process_index++
                )
                {
                    active_process =
                        &monitored_processes[process_index];

                    if (active_process->target_name[0] != '\0')
                    {
                        if (active_process->current_pid > 0)
                        {
                            fprintf(
                                status_stream,
                                "Monitoring process[%zu]: "
                                "%s (PID: %d)\n",
                                process_index,
                                active_process->target_name,
                                active_process->current_pid
                            );
                        }
                        else
                        {
                            fprintf(
                                status_stream,
                                "Waiting for process[%zu]: %s\n",
                                process_index,
                                active_process->target_name
                            );
                        }
                    }
                    else
                    {
                        fprintf(
                            status_stream,
                            "Monitoring process[%zu] PID: %d%s\n",
                            process_index,
                            active_process->current_pid,
                            active_process->configured_pid == 0
                                ? " (self)"
                                : ""
                        );
                    }
                }

            }
        }


        /*
         * 依次完成所有目标进程的一轮监控。
         */
        for (
            process_index = 0;
            process_index < monitored_process_count;
            process_index++
        )
        {
            /*
             * active_process 指向当前正在处理的数组元素。
             */
            active_process =
                &monitored_processes[process_index];

            if (
                monitor_process_once(
                    active_process,
                    &config,
                    status_stream
            ) != 0
            )
            {
                return 1;
            }
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
        if (
            read_network_info_filtered(
                &config,
                &current_network
            ) != 0
        )
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
         * 热加载刚刚修改了网络接口列表时，
         * 当前采样只用于建立稳定的新基准。
         *
         * 不立即使用很短的时间差计算网速，
         * 避免出现不具有代表性的瞬时高值。
         */
        if (network_baseline_reset)
        {
            previous_network =
                current_network;

            previous_network_time =
                current_network_time;

            network_speed.download_bytes_per_sec = 0.0;
            network_speed.upload_bytes_per_sec = 0.0;

            network_baseline_reset = 0;
        }
        else
        {
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
            if (
                calculate_network_speed(
                    &previous_network,
                    &current_network,
                    network_elapsed_seconds,
                    &network_speed
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Failed to calculate network speed\n"
                );

                return 1;
            }
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
         * 将当前一轮已经读取和计算完成的数据
         * 集中放入 MonitorSnapshot。
         */
        snapshot.current_time = current_time;
        snapshot.uptime = system_uptime;
        snapshot.load_average = load_average;

        snapshot.cpu_usage_percent = cpu_usage;
        snapshot.cpu_level = cpu_level;

        snapshot.total_memory_kb =
            memory_info.total_kb;

        snapshot.available_memory_kb =
            memory_info.available_kb;

        snapshot.used_memory_kb =
            memory_info.used_kb;

        snapshot.memory_usage_percent =
            memory_info.usage_percent;

        snapshot.memory_level =
            memory_level;

        snapshot.total_disk_bytes =
            disk_info.total_bytes;

        snapshot.used_disk_bytes =
            disk_info.used_bytes;

        snapshot.available_disk_bytes =
            disk_info.available_bytes;

        snapshot.disk_usage_percent =
            disk_info.usage_percent;

        snapshot.disk_level =
            disk_level;

        snapshot.system_level =
            system_level;

        snapshot.network_receive_total_bytes =
            (unsigned long long)current_network.rx_bytes;

        snapshot.network_transmit_total_bytes =
            (unsigned long long)current_network.tx_bytes;

        snapshot.download_bytes_per_second =
            network_speed.download_bytes_per_sec;

        snapshot.upload_bytes_per_second =
            network_speed.upload_bytes_per_sec;

        /*
         * 不复制进程数组，只保存数组首地址和有效数量。
         */
        snapshot.processes =
            monitored_processes;

        snapshot.process_count =
            monitored_process_count;

        /*
         * JSON 模式直接调用结构化输出模块。
         */
        if (output_format == OUTPUT_FORMAT_JSON)
        {
            if (output_print_json(&snapshot) != 0)
            {
                fprintf(
                    stderr,
                    "Failed to print JSON monitoring output\n"
                );

                return 1;
            }

            /*
             * stdout 被重定向到文件或管道时通常使用缓冲。
             * 主动刷新，确保每一轮 JSON 立即可见。
             */
            fflush(stdout);
        }
        else
        {

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
             * 依次输出所有被监控进程的状态。
             */
            for (
                process_index = 0;
                process_index < monitored_process_count;
                process_index++
            )
            {
                active_process =
                    &monitored_processes[process_index];

                if (active_process->available)
                {
                    printf(
                        "Process[%zu]:       %s PID=%d PPID=%d\n",
                        process_index,
                        active_process->info.name,
                        active_process->info.pid,
                        active_process->info.parent_pid
                    );

                    printf(
                        "  State:           %s\n",
                        active_process->info.state
                    );

                    printf(
                        "  Memory:          %.2f MiB [%s]\n",
                        active_process->memory_mib,
                        alert_level_to_string(
                            active_process->memory_level
                        )
                    );

                    if (active_process->cpu_usage_valid)
                    {
                        printf(
                            "  CPU:             %.2f%% [%s]\n",
                            active_process->cpu_usage,
                            alert_level_to_string(
                                active_process->cpu_level
                            )
                        );
                    }
                    else if (active_process->cpu_sample_initialized)
                    {
                        printf(
                            "  CPU:            [COLLECTING]\n"
                        );
                    }
                    else
                    {
                        printf(
                            "  CPU:            [UNAVAILABLE]\n"
                        );
                    }
                }
                else
                {
                    printf(
                        "Process[%zu]:       %s PID=%d [UNAVAILABLE]\n",
                        process_index,
                        active_process->target_name[0] != '\0'
                            ? active_process->target_name
                            : "(PID target)",
                        active_process->current_pid
                    );
                }
            }

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
        }
        /*
         * 使用名称监控时，如果目标进程已经不可用，
         * 清除旧 PID，让下一轮重新按名称查找。
         */
        if (
            config.process_name[0] != '\0' &&
            !active_process->available
        ) {
            active_process->current_pid = 0;
        }

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
    fprintf(status_stream,"\nEdgeSentinel stopped safely.\n");

    return 0;
}
