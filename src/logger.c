#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "logger.h"

/*
 * 单个日志文件允许的最大大小。
 *
 * 1 MiB = 1024 × 1024 字节。
 */
#define LOG_MAX_SIZE_BYTES (1024LL * 1024LL)

/*
 * 用于保存轮转后日志文件路径的数组大小。
 */
#define ROTATED_PATH_BUFFER_SIZE 4096

/*
 * 检查日志文件大小，并在达到上限时执行轮转。
 *
 * 例如：
 *
 * logs/edgesentinel.log
 *
 * 达到 1 MiB 后重命名为：
 *
 * logs/edgesentinel.log.1
 *
 * 返回值：
 *     成功或无需轮转返回 0；
 *     失败返回 -1。
 *
 * static 表示该函数只在当前 logger.c 中使用，
 * 不向其他源文件公开。
 */
static int rotate_log_if_needed(const char *file_path)
{
    struct stat file_status;

    char rotated_path[ROTATED_PATH_BUFFER_SIZE];

    int path_length;

    /*
     * 防止传入空指针。
     */
    if (file_path == NULL)
    {
        return -1;
    }

    /*
     * stat() 用于读取文件状态，
     * 包括文件大小、权限和修改时间等。
     */
    if (stat(file_path, &file_status) == -1)
    {
        /*
         * ENOENT 表示文件不存在。
         *
         * 日志文件还不存在时，不需要进行轮转，
         * 后续 fopen(..., "a") 会自动创建它。
         */
        if (errno == ENOENT)
        {
            return 0;
        }

        /*
         * 如果是其他错误，则表示读取文件状态失败。
         */
        return -1;
    }

    /*
     * st_size 保存文件当前大小，单位是字节。
     *
     * 文件尚未达到 1 MiB 时，不进行轮转。
     */
    if (file_status.st_size < LOG_MAX_SIZE_BYTES)
    {
        return 0;
    }

    /*
     * 根据原日志文件路径生成轮转文件路径。
     *
     * 例如：
     *
     * file_path：
     *     logs/edgesentinel.log
     *
     * rotated_path：
     *     logs/edgesentinel.log.1
     */
    path_length = snprintf(
        rotated_path,
        sizeof(rotated_path),
        "%s.1",
        file_path
    );

    /*
     * snprintf() 返回负数表示格式化失败。
     *
     * 返回值大于或等于数组容量，
     * 表示生成的路径过长，被截断了。
     */
    if (
        path_length < 0 ||
        (size_t)path_length >= sizeof(rotated_path)
    )
    {
        return -1;
    }

    /*
     * 如果上一次轮转生成的 .1 文件仍然存在，
     * 先删除旧的备份。
     *
     * 这里暂时只保留一份历史日志。
     */
    if (remove(rotated_path) == -1)
    {
        /*
         * ENOENT 表示 .1 文件本来就不存在，
         * 这种情况不属于错误。
         */
        if (errno != ENOENT)
        {
            return -1;
        }
    }

    /*
     * 将当前日志文件重命名为 .1 文件。
     *
     * 例如：
     *
     * edgesentinel.log
     *        ↓
     * edgesentinel.log.1
     */
    if (rename(file_path, rotated_path) == -1)
    {
        return -1;
    }

    return 0;
}

/*
 * 创建日志目录。
 */
int logger_init(const char *directory)
{
    /*
     * 防止调用者传入空指针。
     */
    if (directory == NULL)
    {
        return -1;
    }

    /*
     * 创建目录。
     *
     * 0755 表示：
     *
     * 所有者：读、写、进入目录
     * 其他用户：读、进入目录
     */
    if (mkdir(directory, 0755) == -1)
    {
        /*
         * EEXIST 表示目录已经存在。
         *
         * 日志目录已经存在不属于错误。
         */
        if (errno != EEXIST)
        {
            return -1;
        }
    }

    return 0;
}

/*
 * 向指定文件追加一条带时间和等级的日志。
 */
int logger_write(
    const char *file_path,
    const char *level,
    const char *message
)
{
    FILE *file;

    time_t current_time;
    struct tm local_time;

    char timestamp[20];

    /*
     * 检查参数是否有效。
     */
    if (
        file_path == NULL ||
        level == NULL ||
        message == NULL
    )
    {
        return -1;
    }

    /*
     * 每次写入前先检查日志文件大小。
     *
     * 如果文件达到 1 MiB，
     * rotate_log_if_needed() 会先进行日志轮转。
     */
    if (rotate_log_if_needed(file_path) != 0)
    {
        return -1;
    }

    /*
     * 使用追加模式打开日志文件。
     *
     * "a" 表示 append：
     *
     * 文件存在：
     *     从文件末尾继续写入。
     *
     * 文件不存在：
     *     自动创建新文件。
     *
     * 日志文件刚完成轮转时，
     * 原文件已经被重命名为 .1，
     * 因此这里会创建新的 edgesentinel.log。
     */
    file = fopen(file_path, "a");

    if (file == NULL)
    {
        return -1;
    }

    /*
     * 获取当前系统时间。
     */
    current_time = time(NULL);

    if (current_time == (time_t)-1)
    {
        fclose(file);
        return -1;
    }

    /*
     * 将时间转换成本地时间。
     */
    if (localtime_r(&current_time, &local_time) == NULL)
    {
        fclose(file);
        return -1;
    }

    /*
     * 将时间转换为：
     *
     * 年-月-日 时:分:秒
     */
    if (
        strftime(
            timestamp,
            sizeof(timestamp),
            "%Y-%m-%d %H:%M:%S",
            &local_time
        ) == 0
    )
    {
        fclose(file);
        return -1;
    }

    /*
     * 按统一格式写入日志。
     *
     * 例如：
     *
     * [2026-07-24 22:10:00] [NORMAL] CPU=0.50% ...
     */
    if (
        fprintf(
            file,
            "[%s] [%s] %s\n",
            timestamp,
            level,
            message
        ) < 0
    )
    {
        fclose(file);
        return -1;
    }

    /*
     * 关闭文件，并刷新尚未写入磁盘的数据。
     */
    if (fclose(file) != 0)
    {
        return -1;
    }

    return 0;
}
