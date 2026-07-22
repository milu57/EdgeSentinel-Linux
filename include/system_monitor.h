#ifndef EDGESENTINEL_SYSTEM_MONITOR_H
#define EDGESENTINEL_SYSTEM_MONITOR_H

/*
 * 保存一次内存采样结果。
 *
 * Linux 的 /proc/meminfo 默认使用 kB 作为单位，
 * 因此结构体内部统一保存为 kB。
 */
typedef struct
{
    unsigned long long total_kb;
    unsigned long long available_kb;
    unsigned long long used_kb;
    double usage_percent;
} MemoryInfo;

/*
 * 从 /proc/meminfo 读取系统内存信息。
 *
 * 参数：
 *     info：用于保存读取结果的结构体地址。
 *
 * 返回值：
 *     0：读取成功。
 *    -1：读取失败。
 */
int get_memory_info(MemoryInfo *info);

/*
 * 将内存信息打印到终端。
 */
void print_memory_info(const MemoryInfo *info);

#endif
