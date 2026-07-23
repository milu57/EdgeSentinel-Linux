#ifndef DISK_MONITOR_H
#define DISK_MONITOR_H

/*
 * 保存一个文件系统的磁盘容量信息。
 *
 * 单位统一使用字节 bytes。
 */
typedef struct
{
    unsigned long long total_bytes;
    unsigned long long used_bytes;
    unsigned long long available_bytes;
    double usage_percent;
} DiskInfo;

/*
 * 获取某个路径所在文件系统的磁盘信息。
 *
 * 参数：
 *     path：要查询的路径，例如 "/"
 *     info：保存读取结果的结构体地址
 *
 * 返回值：
 *      0：读取成功
 *     -1：读取失败
 */
int get_disk_info(const char *path, DiskInfo *info);

#endif
