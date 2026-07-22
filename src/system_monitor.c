#include <stdio.h>
#include "system_monitor.h"

int get_memory_info(MemoryInfo *info)
{
    FILE *file;
    char line[256];
    int found_total = 0;
    int found_available = 0;

    /*
     * 防止调用者传入空指针。
     */
    if (info == NULL)
    {
        return -1;
    }

    /*
     * 先把结构体中的数据初始化为 0，
     * 避免读取失败时残留未初始化数据。
     */
    info->total_kb = 0;
    info->available_kb = 0;
    info->used_kb = 0;
    info->usage_percent = 0.0;

    /*
     * /proc/meminfo 是 Linux 内核提供的虚拟文件，
     * 不是真正存储在磁盘上的普通文件。
     */
    file = fopen("/proc/meminfo", "r");

    if (file == NULL)
    {
        return -1;
    }

    /*
     * 每次读取一行，直到文件结束。
     */
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (sscanf(line,
                   "MemTotal: %llu kB",
                   &info->total_kb) == 1)
        {
            found_total = 1;
        }
        else if (sscanf(line,
                        "MemAvailable: %llu kB",
                        &info->available_kb) == 1)
        {
            found_available = 1;
        }

        /*
         * 两项数据都找到后，就不需要继续读取了。
         */
        if (found_total && found_available)
        {
            break;
        }
    }

    fclose(file);

    /*
     * 如果缺少必要字段，说明采样失败。
     */
    if (!found_total || !found_available)
    {
        return -1;
    }

    /*
     * 正常情况下可用内存不应该大于总内存。
     */
    if (info->available_kb > info->total_kb)
    {
        return -1;
    }

    info->used_kb =
        info->total_kb - info->available_kb;

    if (info->total_kb > 0)
    {
        info->usage_percent =
            (double)info->used_kb /
            (double)info->total_kb *
            100.0;
    }

    return 0;
}

void print_memory_info(const MemoryInfo *info)
{
    double total_mb;
    double available_mb;
    double used_mb;

    if (info == NULL)
    {
        return;
    }

    /*
     * 1 MB = 1024 kB
     */
    total_mb = (double)info->total_kb / 1024.0;
    available_mb = (double)info->available_kb / 1024.0;
    used_mb = (double)info->used_kb / 1024.0;

    printf("========== EdgeSentinel-Linux ==========\n");
    printf("Memory total     : %.2f MB\n", total_mb);
    printf("Memory available : %.2f MB\n", available_mb);
    printf("Memory used      : %.2f MB\n", used_mb);
    printf("Memory usage     : %.2f%%\n",
           info->usage_percent);
    printf("========================================\n");
}
