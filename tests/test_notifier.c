#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "alert_event.h"
#include "notifier.h"

/*
 * 检查通知消息中是否包含指定文本。
 */
static int message_contains(
    const char *message,
    const char *expected_text,
    const char *case_name
)
{
    if (strstr(message, expected_text) == NULL) {
        fprintf(
            stderr,
            "%s failed: message does not contain \"%s\"\n",
            case_name,
            expected_text
        );

        return -1;
    }

    return 0;
}

/*
 * 测试 WARNING 告警通知格式。
 */
static int test_warning_message(void)
{
    AlertEvent event;
    char message[NOTIFIER_MESSAGE_LENGTH];

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_SYSTEM_CPU,
            "system",
            ALERT_NORMAL,
            ALERT_WARNING,
            75.30,
            "%"
        ) != 0
    ) {
        fprintf(stderr, "failed to initialize warning event\n");
        return -1;
    }

    if (
        notifier_format_message(
            &event,
            message,
            sizeof(message)
        ) != 0
    ) {
        fprintf(stderr, "failed to format warning message\n");
        return -1;
    }

    if (
        message_contains(
            message,
            "[EdgeSentinel]",
            "notification prefix test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            message,
            "SYSTEM_CPU",
            "metric text test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            message,
            "target=system",
            "target text test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            message,
            "status=NORMAL->WARNING",
            "warning transition test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            message,
            "value=75.30%",
            "warning value test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            message,
            "time=",
            "notification time test"
        ) != 0
    ) {
        return -1;
    }

    printf("warning notification message test passed\n");

    return 0;
}

/*
 * 测试恢复正常通知格式。
 */
static int test_recovery_message(void)
{
    AlertEvent event;
    char message[NOTIFIER_MESSAGE_LENGTH];

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_PROCESS_MEMORY,
            "sleep",
            ALERT_CRITICAL,
            ALERT_NORMAL,
            85.50,
            "MiB"
        ) != 0
    ) {
        fprintf(stderr, "failed to initialize recovery event\n");
        return -1;
    }

    if (
        notifier_format_message(
            &event,
            message,
            sizeof(message)
        ) != 0
    ) {
        fprintf(stderr, "failed to format recovery message\n");
        return -1;
    }

    if (
        message_contains(
            message,
            "PROCESS_MEMORY",
            "recovery metric test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            message,
            "target=sleep",
            "recovery target test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            message,
            "status=CRITICAL->NORMAL",
            "recovery transition test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            message,
            "value=85.50MiB",
            "recovery value test"
        ) != 0
    ) {
        return -1;
    }

    printf("recovery notification message test passed\n");

    return 0;
}

/*
 * 测试非法参数和容量不足的缓冲区。
 */
static int test_invalid_arguments(void)
{
    AlertEvent event;
    char message[NOTIFIER_MESSAGE_LENGTH];
    char small_buffer[16];

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_DISK,
            "/",
            ALERT_WARNING,
            ALERT_CRITICAL,
            92.50,
            "%"
        ) != 0
    ) {
        fprintf(stderr, "failed to initialize disk event\n");
        return -1;
    }

    if (
        notifier_format_message(
            NULL,
            message,
            sizeof(message)
        ) == 0
    ) {
        fprintf(stderr, "NULL event test failed\n");
        return -1;
    }

    if (
        notifier_format_message(
            &event,
            NULL,
            sizeof(message)
        ) == 0
    ) {
        fprintf(stderr, "NULL buffer test failed\n");
        return -1;
    }

    if (
        notifier_format_message(
            &event,
            message,
            0
        ) == 0
    ) {
        fprintf(stderr, "zero buffer size test failed\n");
        return -1;
    }

    /*
     * 16 字节无法保存完整通知消息，
     * notifier_format_message() 应返回失败。
     */
    if (
        notifier_format_message(
            &event,
            small_buffer,
            sizeof(small_buffer)
        ) == 0
    ) {
        fprintf(stderr, "small buffer test failed\n");
        return -1;
    }

    printf("invalid notifier argument tests passed\n");

    return 0;
}

/*
 * 测试通知程序能否从标准输入收到完整消息。
 */
static int test_notifier_send_success(void)
{
    const char *script_path =
        "/tmp/edgesentinel_test_notifier_success.sh";

    const char *output_path =
        "/tmp/edgesentinel_test_notifier_output.txt";

    FILE *script_file;
    FILE *output_file;
    AlertEvent event;
    char received_message[NOTIFIER_MESSAGE_LENGTH];

    /*
     * 创建一个测试脚本。
     *
     * cat 会读取脚本的标准输入，
     * 并把内容写入测试输出文件。
     */
    script_file = fopen(script_path, "w");

    if (script_file == NULL) {
        perror("fopen");
        return -1;
    }

    if (
        fprintf(
            script_file,
            "#!/bin/sh\n"
            "cat > %s\n",
            output_path
        ) < 0
    ) {
        fprintf(stderr, "failed to write notifier test script\n");

        fclose(script_file);
        remove(script_path);

        return -1;
    }

    if (fclose(script_file) != 0) {
        perror("fclose");
        remove(script_path);

        return -1;
    }

    /*
     * 赋予脚本拥有者读取、写入和执行权限。
     */
    if (chmod(script_path, 0700) != 0) {
        perror("chmod");
        remove(script_path);

        return -1;
    }

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_SYSTEM_MEMORY,
            "system",
            ALERT_NORMAL,
            ALERT_WARNING,
            78.25,
            "%"
        ) != 0
    ) {
        fprintf(stderr, "failed to initialize notifier send event\n");

        remove(script_path);
        return -1;
    }

    /*
     * notifier_send() 应启动脚本，
     * 并将格式化后的消息写入脚本标准输入。
     */
    if (
        notifier_send(
            &event,
            script_path
        ) != 0
    ) {
        fprintf(stderr, "notifier_send success test failed\n");

        remove(script_path);
        remove(output_path);

        return -1;
    }

    output_file = fopen(output_path, "r");

    if (output_file == NULL) {
        perror("fopen");

        remove(script_path);
        return -1;
    }

    if (
        fgets(
            received_message,
            sizeof(received_message),
            output_file
        ) == NULL
    ) {
        fprintf(stderr, "failed to read notification output\n");

        fclose(output_file);
        remove(script_path);
        remove(output_path);

        return -1;
    }

    if (fclose(output_file) != 0) {
        perror("fclose");

        remove(script_path);
        remove(output_path);

        return -1;
    }

    if (remove(script_path) != 0) {
        perror("remove");
        remove(output_path);

        return -1;
    }

    if (remove(output_path) != 0) {
        perror("remove");
        return -1;
    }

    if (
        message_contains(
            received_message,
            "SYSTEM_MEMORY",
            "sent notification metric test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            received_message,
            "target=system",
            "sent notification target test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            received_message,
            "status=NORMAL->WARNING",
            "sent notification transition test"
        ) != 0
    ) {
        return -1;
    }

    if (
        message_contains(
            received_message,
            "value=78.25%",
            "sent notification value test"
        ) != 0
    ) {
        return -1;
    }

    printf("notifier send success test passed\n");

    return 0;
}

/*
 * 测试外部通知程序执行失败时，
 * notifier_send() 是否返回错误。
 */
static int test_notifier_send_failures(void)
{
    const char *failure_script_path =
        "/tmp/edgesentinel_test_notifier_failure.sh";

    FILE *script_file;
    AlertEvent event;

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_DISK,
            "/",
            ALERT_WARNING,
            ALERT_CRITICAL,
            95.00,
            "%"
        ) != 0
    ) {
        fprintf(stderr, "failed to initialize failure event\n");
        return -1;
    }

    if (notifier_send(NULL, "/bin/cat") == 0) {
        fprintf(stderr, "NULL event send test failed\n");
        return -1;
    }

    if (notifier_send(&event, NULL) == 0) {
        fprintf(stderr, "NULL command send test failed\n");
        return -1;
    }

    if (notifier_send(&event, "") == 0) {
        fprintf(stderr, "empty command send test failed\n");
        return -1;
    }

    /*
     * 不存在的程序无法被 exec 执行。
     */
    if (
        notifier_send(
            &event,
            "/tmp/edgesentinel-command-does-not-exist"
        ) == 0
    ) {
        fprintf(stderr, "nonexistent command test failed\n");
        return -1;
    }

    /*
     * 创建一个主动返回非零退出状态的脚本。
     */
    script_file = fopen(failure_script_path, "w");

    if (script_file == NULL) {
        perror("fopen");
        return -1;
    }

    if (
        fprintf(
            script_file,
            "#!/bin/sh\n"
            "cat > /dev/null\n"
            "exit 7\n"
        ) < 0
    ) {
        fprintf(stderr, "failed to write failure script\n");

        fclose(script_file);
        remove(failure_script_path);

        return -1;
    }

    if (fclose(script_file) != 0) {
        perror("fclose");
        remove(failure_script_path);

        return -1;
    }

    if (chmod(failure_script_path, 0700) != 0) {
        perror("chmod");
        remove(failure_script_path);

        return -1;
    }

    if (
        notifier_send(
            &event,
            failure_script_path
        ) == 0
    ) {
        fprintf(
            stderr,
            "nonzero notification command exit test failed\n"
        );

        remove(failure_script_path);
        return -1;
    }

    if (remove(failure_script_path) != 0) {
        perror("remove");
        return -1;
    }

    printf("notifier send failure tests passed\n");

    return 0;
}

int main(void)
{
    if (test_warning_message() != 0) {
        return 1;
    }

    if (test_recovery_message() != 0) {
        return 1;
    }

    if (test_invalid_arguments() != 0) {
        return 1;
    }

    if (test_notifier_send_success() != 0) {
        return 1;
    }

    if (test_notifier_send_failures() != 0) {
        return 1;
    }

    printf("all notifier tests passed\n");

    return 0;
}
