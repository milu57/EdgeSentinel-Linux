#include "disk_monitor.h"

#include <stdio.h>
#include <sys/statvfs.h>

int get_disk_info(const char *path, DiskInfo *info)
{
    struct statvfs filesystem;

    unsigned long long block_size;
    unsigned long long free_bytes;
    unsigned long long usage_denominator;

    /*
     * 检查调用者传入的参数是否有效。
     */
    if (path == NULL || info == NULL)
    {
        return -1;
    }

    /*
     * statvfs() 根据 path 查找它所在的文件系统，
     * 并把文件系统容量信息写入 filesystem。
     *
     * 成功返回 0，失败返回 -1。
     */
    if (statvfs(path, &filesystem) == -1)
    {
        perror("statvfs");
        return -1;
    }

    /*
     * f_frsize 表示文件系统中一个块的实际字节数。
     */
    block_size = (unsigned long long)filesystem.f_frsize;

    /*
     * f_blocks：
     * 文件系统的总块数。
     */
    info->total_bytes =
        (unsigned long long)filesystem.f_blocks *
        block_size;

    /*
     * f_bfree：
     * 所有空闲块，包括只允许 root 使用的保留空间。
     */
    free_bytes =
        (unsigned long long)filesystem.f_bfree *
        block_size;

    /*
     * f_bavail：
     * 普通用户真正可以使用的空闲块。
     */
    info->available_bytes =
        (unsigned long long)filesystem.f_bavail *
        block_size;

    /*
     * 已使用空间：
     *
     * 总空间减去所有空闲空间。
     */
    info->used_bytes =
        info->total_bytes - free_bytes;

    /*
     * df 命令的使用率大致按照：
     *
     * used / (used + available)
     *
     * 这样可以考虑文件系统为 root 保留的空间。
     */
    usage_denominator =
        info->used_bytes + info->available_bytes;

    if (usage_denominator == 0)
    {
        info->usage_percent = 0.0;
    }
    else
    {
        info->usage_percent =
            ((double)info->used_bytes /
             (double)usage_denominator) *
            100.0;
    }

    return 0;
}
