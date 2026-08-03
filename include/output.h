#ifndef OUTPUT_H
#define OUTPUT_H

#include <stddef.h>

#include "alert.h"
#include "process_monitor.h"
#include "system_status.h"

/*
 * EdgeSentinel 支持的输出格式。
 *
 * OUTPUT_FORMAT_TEXT：
 *     使用原来的终端文本面板。
 *
 * OUTPUT_FORMAT_JSON：
 *     每一轮采样输出一个 JSON 对象。
 */
typedef enum
{
    OUTPUT_FORMAT_TEXT = 0,
    OUTPUT_FORMAT_JSON
} OutputFormat;

/*
 * 保存一轮系统监控产生的完整结果。
 *
 * 这个结构体不负责读取 /proc，也不负责计算告警等级。
 * 它只负责把各个监控模块已经得到的结果集中起来，
 * 再交给文本输出函数或 JSON 输出函数。
 */
typedef struct
{
    /*
     * 当前时间。
     */
    CurrentTime current_time;

    /*
     * 系统运行时间。
     */
    SystemUptime uptime;

    /*
     * 1、5、15 分钟系统平均负载。
     */
    LoadAverage load_average;

    /*
     * 系统 CPU 使用率及告警等级。
     */
    double cpu_usage_percent;
    AlertLevel cpu_level;

    /*
     * 内存容量与 MemoryInfo 保持一致，
     * 单位统一为 kB。
     */
    unsigned long long total_memory_kb;
    unsigned long long available_memory_kb;
    unsigned long long used_memory_kb;
    double memory_usage_percent;
    AlertLevel memory_level;

    /*
     * 根文件系统磁盘信息。
     */
    unsigned long long total_disk_bytes;
    unsigned long long used_disk_bytes;
    unsigned long long available_disk_bytes;
    double disk_usage_percent;
    AlertLevel disk_level;

    /*
     * CPU、内存和磁盘中的最高告警等级。
     */
    AlertLevel system_level;

    /*
     * 当前累计网络流量。
     */
    unsigned long long network_receive_total_bytes;
    unsigned long long network_transmit_total_bytes;

    /*
     * 当前实时网络速度。
     *
     * JSON 中统一使用 B/s，
     * 不在数据结构中提前转换成 KB/s 或 MB/s。
     */
    double download_bytes_per_second;
    double upload_bytes_per_second;

    /*
     * 指向 main.c 中的被监控进程数组。
     *
     * 这里只保存数组地址，不复制整个数组。
     */
    const MonitoredProcess *processes;

    /*
     * processes 数组中实际有效的进程数量。
     */
    size_t process_count;
} MonitorSnapshot;

/*
 * 将字符串转换为输出格式。
 *
 * 支持：
 *     "text"
 *     "json"
 *
 * 成功返回 0，失败返回 -1。
 */
int output_parse_format(
    const char *text,
    OutputFormat *format
);

/*
 * 将输出格式转换为字符串。
 *
 * 例如：
 *     OUTPUT_FORMAT_TEXT -> "text"
 *     OUTPUT_FORMAT_JSON -> "json"
 */
const char *output_format_to_string(
    OutputFormat format
);

/*
 * 使用原来的文本形式输出一轮监控结果。
 *
 * 成功返回 0，失败返回 -1。
 */
int output_print_text(
    const MonitorSnapshot *snapshot
);

/*
 * 使用 JSON 形式输出一轮监控结果。
 *
 * 成功返回 0，失败返回 -1。
 */
int output_print_json(
    const MonitorSnapshot *snapshot
);

/*
 * 根据 format 自动选择文本输出或 JSON 输出。
 *
 * 成功返回 0，失败返回 -1。
 */
int output_print(
    const MonitorSnapshot *snapshot,
    OutputFormat format
);

#endif
