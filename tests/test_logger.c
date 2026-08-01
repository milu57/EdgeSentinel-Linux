#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "logger.h"

#define TEST_DIRECTORY \
    "/tmp/edgesentinel_logger_test"

#define TEST_NESTED_DIRECTORY \
    "/tmp/edgesentinel_logger_test/nested"

#define TEST_LOG_FILE \
    "/tmp/edgesentinel_logger_test/nested/test.log"

#define TEST_LOG_MESSAGE \
    "logger unit test message"

/*
 * 删除测试产生的日志文件和目录。
 *
 * 测试开始前调用一次，清理上次可能留下的文件；
 * 测试结束后再调用一次，避免污染系统。
 */
static void cleanup_test_files(void)
{
    remove(TEST_LOG_FILE);
    remove(TEST_LOG_FILE ".1");

    rmdir(TEST_NESTED_DIRECTORY);
    rmdir(TEST_DIRECTORY);
}

/*
 * 测试日志目录创建和日志内容写入。
 */
static int test_logger_write(void)
{
    struct stat directory_status;
    FILE *file;
    char log_line[512];

    cleanup_test_files();

    /*
     * 初始化日志模块。
     *
     * TEST_NESTED_DIRECTORY 当前并不存在，
     * logger_init() 应自动创建两层目录。
     */
    if (logger_init(TEST_LOG_FILE, 4096UL) != 0) {
        fprintf(stderr, "logger_init failed\n");

        cleanup_test_files();
        return -1;
    }

    /*
     * 确认嵌套目录已经创建，
     * 并且它确实是目录。
     */
    if (
        stat(
            TEST_NESTED_DIRECTORY,
            &directory_status
        ) != 0 ||
        !S_ISDIR(directory_status.st_mode)
    ) {
        fprintf(
            stderr,
            "logger_init did not create the log directory\n"
        );

        cleanup_test_files();
        return -1;
    }

    /*
     * 写入一条测试日志。
     */
    if (
        logger_write(
            TEST_LOG_FILE,
            "INFO",
            TEST_LOG_MESSAGE
        ) != 0
    ) {
        fprintf(stderr, "logger_write failed\n");

        cleanup_test_files();
        return -1;
    }

    /*
     * 打开 logger_write() 创建的日志文件。
     */
    file = fopen(TEST_LOG_FILE, "r");

    if (file == NULL) {
        perror("fopen");

        cleanup_test_files();
        return -1;
    }

    /*
     * 读取第一行日志。
     */
    if (
        fgets(
            log_line,
            sizeof(log_line),
            file
        ) == NULL
    ) {
        fprintf(stderr, "failed to read the log file\n");

        fclose(file);
        cleanup_test_files();

        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose");

        cleanup_test_files();
        return -1;
    }

    /*
     * 时间会随着测试运行时间变化，
     * 所以不检查具体时间，只检查固定内容。
     */
    if (strstr(log_line, "[INFO]") == NULL) {
        fprintf(
            stderr,
            "log line does not contain INFO level: %s",
            log_line
        );

        cleanup_test_files();
        return -1;
    }

    if (
        strstr(
            log_line,
            TEST_LOG_MESSAGE
        ) == NULL
    ) {
        fprintf(
            stderr,
            "log line does not contain the message: %s",
            log_line
        );

        cleanup_test_files();
        return -1;
    }

    cleanup_test_files();

    printf("logger directory and write test passed\n");

    return 0;
}


/*
 * 检查指定文件中是否包含目标文本。
 *
 * 返回：
 *     1  包含
 *     0  不包含或读取失败
 */
static int file_contains_text(
    const char *file_path,
    const char *expected_text
)
{
    FILE *file;
    char content[2048];
    size_t bytes_read;

    file = fopen(file_path, "r");

    if (file == NULL) {
        return 0;
    }

    bytes_read = fread(
        content,
        1,
        sizeof(content) - 1,
        file
    );

    if (ferror(file)) {
        fclose(file);
        return 0;
    }

    content[bytes_read] = '\0';

    if (fclose(file) != 0) {
        return 0;
    }

    return strstr(content, expected_text) != NULL;
}

/*
 * 测试 logger_init() 和 logger_write()
 * 是否会拒绝非法参数。
 */
static int test_logger_invalid_arguments(void)
{
    cleanup_test_files();

    if (logger_init(NULL, 4096UL) == 0) {
        fprintf(
            stderr,
            "logger_init accepted NULL path\n"
        );

        return -1;
    }

    if (logger_init("", 4096UL) == 0) {
        fprintf(
            stderr,
            "logger_init accepted empty path\n"
        );

        return -1;
    }

    if (logger_init(TEST_LOG_FILE, 0UL) == 0) {
        fprintf(
            stderr,
            "logger_init accepted zero maximum size\n"
        );

        return -1;
    }

    /*
     * 路径以斜杠结尾，只有目录，没有文件名。
     */
    if (
        logger_init(
            TEST_NESTED_DIRECTORY "/",
            4096UL
        ) == 0
    ) {
        fprintf(
            stderr,
            "logger_init accepted path without file name\n"
        );

        return -1;
    }

    if (
        logger_write(
            NULL,
            "INFO",
            "message"
        ) == 0
    ) {
        fprintf(
            stderr,
            "logger_write accepted NULL path\n"
        );

        return -1;
    }

    if (
        logger_write(
            TEST_LOG_FILE,
            NULL,
            "message"
        ) == 0
    ) {
        fprintf(
            stderr,
            "logger_write accepted NULL level\n"
        );

        return -1;
    }

    if (
        logger_write(
            TEST_LOG_FILE,
            "INFO",
            NULL
        ) == 0
    ) {
        fprintf(
            stderr,
            "logger_write accepted NULL message\n"
        );

        return -1;
    }

    cleanup_test_files();

    printf("logger invalid argument tests passed\n");

    return 0;
}

/*
 * 测试日志文件达到大小上限后是否执行轮转。
 */
static int test_logger_rotation(void)
{
    const char *first_message =
        "first message that makes the log file "
        "larger than the configured limit";

    const char *second_message =
        "second message written after rotation";

    cleanup_test_files();

    /*
     * 设置较小的日志上限。
     *
     * 第一条日志写入后会超过 64 字节；
     * 第二次写日志前，logger_write() 会检查文件大小，
     * 然后把旧文件改名为 test.log.1。
     */
    if (logger_init(TEST_LOG_FILE, 64UL) != 0) {
        fprintf(
            stderr,
            "logger_init failed for rotation test\n"
        );

        cleanup_test_files();
        return -1;
    }

    if (
        logger_write(
            TEST_LOG_FILE,
            "INFO",
            first_message
        ) != 0
    ) {
        fprintf(
            stderr,
            "failed to write first rotation message\n"
        );

        cleanup_test_files();
        return -1;
    }

    if (
        logger_write(
            TEST_LOG_FILE,
            "WARNING",
            second_message
        ) != 0
    ) {
        fprintf(
            stderr,
            "failed to write second rotation message\n"
        );

        cleanup_test_files();
        return -1;
    }

    /*
     * 旧日志应保存到 test.log.1。
     */
    if (
        !file_contains_text(
            TEST_LOG_FILE ".1",
            first_message
        )
    ) {
        fprintf(
            stderr,
            "rotated log does not contain first message\n"
        );

        cleanup_test_files();
        return -1;
    }

    /*
     * 新日志文件应包含第二条消息。
     */
    if (
        !file_contains_text(
            TEST_LOG_FILE,
            second_message
        )
    ) {
        fprintf(
            stderr,
            "new log does not contain second message\n"
        );

        cleanup_test_files();
        return -1;
    }

    /*
     * 第一条消息不应该继续留在新的日志文件中。
     */
    if (
        file_contains_text(
            TEST_LOG_FILE,
            first_message
        )
    ) {
        fprintf(
            stderr,
            "first message remained in the new log file\n"
        );

        cleanup_test_files();
        return -1;
    }

    cleanup_test_files();

    printf("logger rotation test passed\n");

    return 0;
}

int main(void)
{
    if (test_logger_write() != 0) {
        fprintf(stderr, "logger write test failed\n");
        return 1;
    }

    if (test_logger_invalid_arguments() != 0) {
        fprintf(
            stderr,
            "logger invalid argument tests failed\n"
        );

        return 1;
    }

    if (test_logger_rotation() != 0) {
        fprintf(
            stderr,
            "logger rotation test failed\n"
        );

        return 1;
    }

    printf("all logger tests passed\n");

    return 0;
}
