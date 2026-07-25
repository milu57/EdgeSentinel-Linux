#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_PATH_LENGTH 256

/*
 * EdgeSentinel-Linux 全局配置
 */
typedef struct {
    /* 监控采样间隔，单位：秒 */
    unsigned int monitor_interval;

    /* CPU 告警阈值 */
    double cpu_warning_threshold;
    double cpu_critical_threshold;

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
