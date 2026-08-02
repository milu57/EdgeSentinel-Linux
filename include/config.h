#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_PATH_LENGTH 256
#define CONFIG_MAX_PROCESS_NAMES 8
#define CONFIG_PROCESS_NAME_LENGTH 256
#define CONFIG_MAX_NETWORK_INTERFACES 8
#define CONFIG_NETWORK_INTERFACE_NAME_LENGTH 32


/*
 * EdgeSentinel-Linux 全局配置
 */
typedef struct {
    /* 监控采样间隔，单位：秒 */
    unsigned int monitor_interval;

    /*
     * 要监控的进程 PID。
     *
     * 0：
     *     监控 EdgeSentinel 自身。
     *
     * 大于 0：
     *     监控指定 PID 的进程。
     */
    unsigned int process_pid;
    /*
     * 旧的单进程名称配置。
     *
     * 暂时保留，用于兼容当前代码；
     * 后续迁移完成后再删除。
     */
    char process_name[CONFIG_PROCESS_NAME_LENGTH];

    /*
     * 多进程名称列表。
     *
     * process_names[0] 表示第一个进程名称；
     * process_names[1] 表示第二个进程名称；
     * 以此类推。
     */
    char process_names
        [CONFIG_MAX_PROCESS_NAMES]
        [CONFIG_PROCESS_NAME_LENGTH];

    /*
     * process_names 数组中实际保存了多少个名称。
     */
    unsigned int process_name_count;

    /*
     * 需要监控的网络接口名称列表。
     *
     * network_interface_count == 0：
     *     统计所有非回环网络接口。
     *
     * network_interface_count > 0：
     *     只统计列表中指定的网络接口。
     */
    char network_interfaces
        [CONFIG_MAX_NETWORK_INTERFACES]
        [CONFIG_NETWORK_INTERFACE_NAME_LENGTH];

    /*
     * network_interfaces 数组中实际保存的接口数量。
     */
    unsigned int network_interface_count;

    /* CPU 告警阈值 */
    double cpu_warning_threshold;
    double cpu_critical_threshold;

    /*
     * 被监控进程的 CPU 使用率告警阈值。
     *
     * 当前采用“一个 CPU 核心为 100%”的定义。
    */
    double process_cpu_warning_threshold;
    double process_cpu_critical_threshold;

    /*
     * 被监控进程的常驻内存告警阈值。
     */

    double process_memory_warning_threshold_mib;
    double process_memory_critical_threshold_mib;

    /* 内存告警阈值 */
    double memory_warning_threshold;
    double memory_critical_threshold;

    /* 磁盘告警阈值 */
    double disk_warning_threshold;
    double disk_critical_threshold;

    /* 日志文件路径 */
    char log_file[CONFIG_PATH_LENGTH];

    /* 日志轮转大小，单位：字节 */
    unsigned long log_max_size;
} AppConfig;

/*
 * 为配置结构体填入默认值
 */
void config_set_defaults(AppConfig *config);

/*
 * 从配置文件读取配置
 *
 * 成功返回 0
 * 失败返回 -1
 */
int config_load(const char *filename, AppConfig *config);

/*
 * 检查配置是否合法。
 *
 * 合法返回 0；
 * 非法返回 -1。
 */
int config_validate(const AppConfig *config);

/*
 * 打印当前配置，用于调试
 */
void config_print(const AppConfig *config);

#endif
