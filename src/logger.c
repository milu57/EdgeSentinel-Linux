#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "logger.h"

/*
 * 日志路径缓冲区大小。
 */
#define LOGGER_PATH_BUFFER_SIZE 4096

/*
 * 如果 main.c 没有调用 logger_init()，
 * 日志轮转默认仍然使用 1 MiB。
 */
#define LOGGER_DEFAULT_MAX_SIZE_BYTES (1024UL * 1024UL)

/*
 * 当前生效的日志轮转大小。
 *
 * static 表示该变量只允许在 logger.c 内部使用，
 * 不会暴露给其他源文件。
 */
static unsigned long current_log_max_size =
    LOGGER_DEFAULT_MAX_SIZE_BYTES;

/*
 * 创建目录以及目录中的中间层级。
 *
 * 例如：
 *
 *     logs
 *
 * 或者：
 *
 *     output/logs/edgesentinel
 *
 * 如果目录已经存在，也视为成功。
 */
static int create_directories(const char *directory_path)
{
    char path_buffer[LOGGER_PATH_BUFFER_SIZE];
    char *current;
    size_t path_length;

    if (directory_path == NULL || *directory_path == '\0') {
        return -1;
    }

    path_length = strlen(directory_path);

    if (path_length >= sizeof(path_buffer)) {
        fprintf(stderr, "Log directory path is too long\n");
        return -1;
    }

    /*
     * 把目录路径复制到可修改的数组中。
     */
    snprintf(
        path_buffer,
        sizeof(path_buffer),
        "%s",
        directory_path
    );

    /*
     * 删除路径末尾多余的斜杠。
     *
     * 例如：
     *
     *     logs/
     *
     * 处理为：
     *
     *     logs
     */
    path_length = strlen(path_buffer);

    while (
        path_length > 1 &&
        path_buffer[path_length - 1] == '/'
    ) {
        path_buffer[path_length - 1] = '\0';
        path_length--;
    }

    /*
     * 从左到右检查路径中的每一个斜杠，
     * 逐级创建目录。
     */
    current = path_buffer + 1;

    while (*current != '\0') {
        if (*current == '/') {
            /*
             * 暂时把斜杠改成字符串结束符，
             * 得到当前这一层目录。
             */
            *current = '\0';

            if (
                mkdir(path_buffer, 0755) != 0 &&
                errno != EEXIST
            ) {
                perror("mkdir");
                return -1;
            }

            /*
             * 恢复原来的斜杠，
             * 继续处理下一层目录。
             */
            *current = '/';
        }

        current++;
    }

    /*
     * 创建最后一级目录。
     */
    if (
        mkdir(path_buffer, 0755) != 0 &&
        errno != EEXIST
    ) {
        perror("mkdir");
        return -1;
    }

    return 0;
}

/*
 * 检查日志文件大小，并在达到上限时执行轮转。
 *
 * 例如：
 *
 *     logs/edgesentinel.log
 *
 * 达到上限后重命名为：
 *
 *     logs/edgesentinel.log.1
 */
static int rotate_log_if_needed(const char *file_path)
{
    struct stat file_status;
    char rotated_path[LOGGER_PATH_BUFFER_SIZE];
    int path_length;

    if (file_path == NULL) {
        return -1;
    }

    /*
     * 读取当前日志文件状态。
     */
    if (stat(file_path, &file_status) != 0) {
        /*
         * 日志文件尚不存在时，不需要轮转。
         * 后续 fopen(..., "a") 会自动创建文件。
         */
        if (errno == ENOENT) {
            return 0;
        }

        perror("stat");
        return -1;
    }

    /*
     * 文件还没有达到配置中的最大大小。
     */
    if (
        file_status.st_size <
        (off_t)current_log_max_size
    ) {
        return 0;
    }

    /*
     * 构造轮转后的文件名。
     */
    path_length = snprintf(
        rotated_path,
        sizeof(rotated_path),
        "%s.1",
        file_path
    );

    if (
        path_length < 0 ||
        (size_t)path_length >= sizeof(rotated_path)
    ) {
        fprintf(stderr, "Rotated log path is too long\n");
        return -1;
    }

    /*
     * rename() 会把原日志文件改名为 .1。
     *
     * 原来的日志文件不存在后，
     * 下一次 fopen(..., "a") 会重新创建一个新文件。
     */
    if (rename(file_path, rotated_path) != 0) {
        perror("rename");
        return -1;
    }

    return 0;
}

/*
 * 初始化日志模块。
 */
int logger_init(
    const char *file_path,
    unsigned long max_size_bytes
)
{
    char directory_path[LOGGER_PATH_BUFFER_SIZE];
    const char *last_slash;
    size_t directory_length;

    if (file_path == NULL || *file_path == '\0') {
        fprintf(stderr, "Invalid log file path\n");
        return -1;
    }

    if (max_size_bytes == 0) {
        fprintf(stderr, "Log maximum size must be greater than 0\n");
        return -1;
    }

    /*
     * 保存配置文件中读取到的轮转大小。
     */
    current_log_max_size = max_size_bytes;

    /*
     * 查找日志路径中的最后一个斜杠。
     *
     * 例如：
     *
     *     logs/edgesentinel.log
     *         ↑
     *
     * 最后一个斜杠左侧是目录，
     * 右侧是日志文件名。
     */
    last_slash = strrchr(file_path, '/');

    /*
     * 如果路径中没有斜杠，例如：
     *
     *     edgesentinel.log
     *
     * 说明日志写在当前目录，不需要创建目录。
     */
    if (last_slash == NULL) {
        return 0;
    }

    /*
     * 路径不能以斜杠结尾。
     *
     * 例如下面不是有效日志文件路径：
     *
     *     logs/
     */
    if (*(last_slash + 1) == '\0') {
        fprintf(stderr, "Log path does not contain a file name\n");
        return -1;
    }

    directory_length = (size_t)(last_slash - file_path);

    /*
     * 路径为 /edgesentinel.log 时，
     * 日志目录是根目录 /，不需要创建。
     */
    if (directory_length == 0) {
        return 0;
    }

    if (directory_length >= sizeof(directory_path)) {
        fprintf(stderr, "Log directory path is too long\n");
        return -1;
    }

    memcpy(
        directory_path,
        file_path,
        directory_length
    );

    directory_path[directory_length] = '\0';

    return create_directories(directory_path);
}

/*
 * 向日志文件追加一条日志。
 */
int logger_write(
    const char *file_path,
    const char *level,
    const char *message
)
{
    FILE *file;
    time_t current_time;
    struct tm *local_time;
    char time_text[32];

    if (
        file_path == NULL ||
        level == NULL ||
        message == NULL
    ) {
        return -1;
    }

    /*
     * 每次写日志前检查是否需要轮转。
     */
    if (rotate_log_if_needed(file_path) != 0) {
        return -1;
    }

    /*
     * 以追加模式打开日志文件。
     *
     * 文件不存在时自动创建；
     * 文件存在时从末尾继续写入。
     */
    file = fopen(file_path, "a");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    current_time = time(NULL);

    if (current_time == (time_t)-1) {
        fclose(file);
        return -1;
    }

    local_time = localtime(&current_time);

    if (local_time == NULL) {
        fclose(file);
        return -1;
    }

    if (
        strftime(
            time_text,
            sizeof(time_text),
            "%Y-%m-%d %H:%M:%S",
            local_time
        ) == 0
    ) {
        fclose(file);
        return -1;
    }

    /*
     * 日志格式：
     *
     * [2026-07-25 10:30:00] [INFO] EdgeSentinel started
     */
    if (
        fprintf(
            file,
            "[%s] [%s] %s\n",
            time_text,
            level,
            message
        ) < 0
    ) {
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose");
        return -1;
    }

    return 0;
}
