#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "alert_event.h"

/*
 * 测试 AlertMetric 枚举值转换成字符串。
 */
static int test_alert_metric_strings(void)
{
    if (
        strcmp(
            alert_metric_to_string(ALERT_METRIC_SYSTEM_CPU),
            "SYSTEM_CPU"
        ) != 0
    ) {
        fprintf(stderr, "system CPU metric string test failed\n");
        return -1;
    }

    if (
        strcmp(
            alert_metric_to_string(ALERT_METRIC_SYSTEM_MEMORY),
            "SYSTEM_MEMORY"
        ) != 0
    ) {
        fprintf(stderr, "system memory metric string test failed\n");
        return -1;
    }

    if (
        strcmp(
            alert_metric_to_string(ALERT_METRIC_DISK),
            "DISK"
        ) != 0
    ) {
        fprintf(stderr, "disk metric string test failed\n");
        return -1;
    }

    if (
        strcmp(
            alert_metric_to_string(ALERT_METRIC_PROCESS_CPU),
            "PROCESS_CPU"
        ) != 0
    ) {
        fprintf(stderr, "process CPU metric string test failed\n");
        return -1;
    }

    if (
        strcmp(
            alert_metric_to_string(ALERT_METRIC_PROCESS_MEMORY),
            "PROCESS_MEMORY"
        ) != 0
    ) {
        fprintf(stderr, "process memory metric string test failed\n");
        return -1;
    }

    /*
     * 不属于 AlertMetric 的枚举值应返回 UNKNOWN。
     */
    if (
        strcmp(
            alert_metric_to_string((AlertMetric)99),
            "UNKNOWN"
        ) != 0
    ) {
        fprintf(stderr, "unknown metric string test failed\n");
        return -1;
    }

    printf("alert metric string tests passed\n");

    return 0;
}

/*
 * 测试正常初始化告警事件。
 */
static int test_alert_event_initialization(void)
{
    AlertEvent event;
    time_t before_initialization;
    time_t after_initialization;

    before_initialization = time(NULL);

    if (before_initialization == (time_t)-1) {
        fprintf(stderr, "failed to read time before initialization\n");
        return -1;
    }

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
        fprintf(stderr, "alert event initialization failed\n");
        return -1;
    }

    after_initialization = time(NULL);

    if (after_initialization == (time_t)-1) {
        fprintf(stderr, "failed to read time after initialization\n");
        return -1;
    }

    if (event.metric != ALERT_METRIC_SYSTEM_CPU) {
        fprintf(stderr, "alert event metric test failed\n");
        return -1;
    }

    if (strcmp(event.target, "system") != 0) {
        fprintf(stderr, "alert event target test failed\n");
        return -1;
    }

    if (event.previous_level != ALERT_NORMAL) {
        fprintf(stderr, "previous alert level test failed\n");
        return -1;
    }

    if (event.current_level != ALERT_WARNING) {
        fprintf(stderr, "current alert level test failed\n");
        return -1;
    }

    if (event.current_value != 75.30) {
        fprintf(stderr, "alert event current value test failed\n");
        return -1;
    }

    if (strcmp(event.unit, "%") != 0) {
        fprintf(stderr, "alert event unit test failed\n");
        return -1;
    }

    /*
     * occurred_at 应位于调用函数前后取得的两个时间点之间。
     */
    if (
        event.occurred_at < before_initialization ||
        event.occurred_at > after_initialization
    ) {
        fprintf(stderr, "alert event occurrence time test failed\n");
        return -1;
    }

    printf("alert event initialization test passed\n");

    return 0;
}

/*
 * 测试非法参数是否会被拒绝。
 */
static int test_invalid_alert_event_arguments(void)
{
    AlertEvent event;

    char oversized_target[ALERT_EVENT_TARGET_LENGTH + 1];
    char oversized_unit[ALERT_EVENT_UNIT_LENGTH + 1];

    /*
     * 构造长度正好等于目标数组容量的字符串。
     *
     * 因为还需要保存 '\0'，
     * 所以该字符串必须被拒绝。
     */
    memset(
        oversized_target,
        'a',
        ALERT_EVENT_TARGET_LENGTH
    );

    oversized_target[ALERT_EVENT_TARGET_LENGTH] = '\0';

    memset(
        oversized_unit,
        'b',
        ALERT_EVENT_UNIT_LENGTH
    );

    oversized_unit[ALERT_EVENT_UNIT_LENGTH] = '\0';

    if (
        alert_event_init(
            NULL,
            ALERT_METRIC_SYSTEM_CPU,
            "system",
            ALERT_NORMAL,
            ALERT_WARNING,
            75.30,
            "%"
        ) == 0
    ) {
        fprintf(stderr, "NULL event test failed\n");
        return -1;
    }

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_SYSTEM_CPU,
            NULL,
            ALERT_NORMAL,
            ALERT_WARNING,
            75.30,
            "%"
        ) == 0
    ) {
        fprintf(stderr, "NULL target test failed\n");
        return -1;
    }

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_SYSTEM_CPU,
            "system",
            ALERT_NORMAL,
            ALERT_WARNING,
            75.30,
            NULL
        ) == 0
    ) {
        fprintf(stderr, "NULL unit test failed\n");
        return -1;
    }

    if (
        alert_event_init(
            &event,
            (AlertMetric)99,
            "system",
            ALERT_NORMAL,
            ALERT_WARNING,
            75.30,
            "%"
        ) == 0
    ) {
        fprintf(stderr, "invalid metric test failed\n");
        return -1;
    }

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_SYSTEM_CPU,
            "system",
            (AlertLevel)99,
            ALERT_WARNING,
            75.30,
            "%"
        ) == 0
    ) {
        fprintf(stderr, "invalid previous level test failed\n");
        return -1;
    }

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_SYSTEM_CPU,
            "system",
            ALERT_NORMAL,
            (AlertLevel)99,
            75.30,
            "%"
        ) == 0
    ) {
        fprintf(stderr, "invalid current level test failed\n");
        return -1;
    }

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_SYSTEM_CPU,
            oversized_target,
            ALERT_NORMAL,
            ALERT_WARNING,
            75.30,
            "%"
        ) == 0
    ) {
        fprintf(stderr, "oversized target test failed\n");
        return -1;
    }

    if (
        alert_event_init(
            &event,
            ALERT_METRIC_SYSTEM_CPU,
            "system",
            ALERT_NORMAL,
            ALERT_WARNING,
            75.30,
            oversized_unit
        ) == 0
    ) {
        fprintf(stderr, "oversized unit test failed\n");
        return -1;
    }

    printf("invalid alert event argument tests passed\n");

    return 0;
}

int main(void)
{
    if (test_alert_metric_strings() != 0) {
        return 1;
    }

    if (test_alert_event_initialization() != 0) {
        return 1;
    }

    if (test_invalid_alert_event_arguments() != 0) {
        return 1;
    }

    printf("all alert event tests passed\n");

    return 0;
}
