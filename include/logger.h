#ifndef LOGGER_H
#define LOGGER_H

/*
 * 初始化日志模块。
 *
 * file_path:
 *     日志文件完整路径，例如：
 *
 *     logs/edgesentinel.log
 *
 * max_size_bytes:
 *     单个日志文件允许的最大大小，单位为字节。
 *
 * 功能：
 *     1. 根据 file_path 创建日志目录；
 *     2. 保存日志轮转大小；
 *     3. 为后续 logger_write() 做准备。
 *
 * 返回值：
 *     成功返回 0；
 *     失败返回 -1。
 */
int logger_init(
    const char *file_path,
    unsigned long max_size_bytes
);

/*
 * 向日志文件追加一条日志。
 *
 * file_path:
 *     日志文件路径。
 *
 * level:
 *     日志等级，例如 "INFO"、"WARNING"、"CRITICAL"。
 *
 * message:
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
