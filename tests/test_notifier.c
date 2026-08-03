#include <stddef.h>
#include <stdio.h>
#include <string.h>

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

    printf("all notifier tests passed\n");

    return 0;
}
