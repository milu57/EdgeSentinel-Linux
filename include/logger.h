#ifndef LOGGER_H
#define LOGGER_H

/*
 * 创建日志目录。
 *
 * directory：
 *     需要创建的目录路径，例如 "logs"。
 *
 * 返回值：
 *     成功返回 0；
 *     失败返回 -1。
 */
int logger_init(const char *directory);

/*
 * 向日志文件追加一条日志。
 *
 * file_path：
 *     日志文件路径。
 *
 * level：
 *     日志等级，例如 "INFO"、"WARNING"。
 *
 * message：
 *     需要写入的日志内容。
 *
 * 返回值：
 *     成功返回 0；
 *     失败返回 -1。
 */
int logger_write(
    const char *file_path,
    const char *level,
    const char *message
);

#endif
