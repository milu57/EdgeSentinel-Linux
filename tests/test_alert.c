#include <stdio.h>
#include <string.h>

#include "alert.h"

/*
 * 检查一次告警等级判断结果。
 *
 * actual：
 *     alert_evaluate_percentage() 实际返回的等级。
 *
 * expected：
 *     测试预期得到的等级。
 */
static int check_alert_level(
    const char *case_name,
    AlertLevel actual,
    AlertLevel expected
)
{
    if (actual != expected) {
        fprintf(
            stderr,
            "%s failed: expected %s, got %s\n",
            case_name,
            alert_level_to_string(expected),
            alert_level_to_string(actual)
        );

        return -1;
    }

    printf("%s passed\n", case_name);

    return 0;
}


/*
 * 测试告警等级转换为字符串。
 */
static int test_alert_level_strings(void)
{
    if (
        strcmp(
            alert_level_to_string(ALERT_NORMAL),
            "NORMAL"
        ) != 0
    ) {
        fprintf(stderr, "ALERT_NORMAL string test failed\n");
        return -1;
    }

    if (
        strcmp(
            alert_level_to_string(ALERT_WARNING),
            "WARNING"
        ) != 0
    ) {
        fprintf(stderr, "ALERT_WARNING string test failed\n");
        return -1;
    }

    if (
        strcmp(
            alert_level_to_string(ALERT_CRITICAL),
            "CRITICAL"
        ) != 0
    ) {
        fprintf(stderr, "ALERT_CRITICAL string test failed\n");
        return -1;
    }

    /*
     * 非法枚举值应该转换为 UNKNOWN。
     */
    if (
        strcmp(
            alert_level_to_string((AlertLevel)99),
            "UNKNOWN"
        ) != 0
    ) {
        fprintf(stderr, "unknown alert level string test failed\n");
        return -1;
    }

    printf("alert level string tests passed\n");

    return 0;
}

/*
 * 测试从两个告警等级中选择更严重的等级。
 */
static int test_higher_alert_level(void)
{
    if (
        alert_get_higher_level(
            ALERT_NORMAL,
            ALERT_WARNING
        ) != ALERT_WARNING
    ) {
        fprintf(stderr, "NORMAL and WARNING comparison failed\n");
        return -1;
    }

    if (
        alert_get_higher_level(
            ALERT_CRITICAL,
            ALERT_WARNING
        ) != ALERT_CRITICAL
    ) {
        fprintf(stderr, "CRITICAL and WARNING comparison failed\n");
        return -1;
    }

    if (
        alert_get_higher_level(
            ALERT_WARNING,
            ALERT_WARNING
        ) != ALERT_WARNING
    ) {
        fprintf(stderr, "equal alert level comparison failed\n");
        return -1;
    }

    /*
     * 同时测试参数顺序交换后结果仍然正确。
     */
    if (
        alert_get_higher_level(
            ALERT_WARNING,
            ALERT_CRITICAL
        ) != ALERT_CRITICAL
    ) {
        fprintf(stderr, "reversed alert level comparison failed\n");
        return -1;
    }

    printf("higher alert level tests passed\n");

    return 0;
}

int main(void)
{
    /*
     * 设定：
     *
     * warning  = 70
     * critical = 90
     */

    /*
     * 69.9 还没有达到 warning，
     * 因此应该是 NORMAL。
     */
    if (
        check_alert_level(
            "normal level test",
            alert_evaluate_percentage(
                69.9,
                70.0,
                90.0
            ),
            ALERT_NORMAL
        ) != 0
    ) {
        return 1;
    }

    /*
     * 70.0 正好达到 warning，
     * 因此应该是 WARNING。
     */
    if (
        check_alert_level(
            "warning boundary test",
            alert_evaluate_percentage(
                70.0,
                70.0,
                90.0
            ),
            ALERT_WARNING
        ) != 0
    ) {
        return 1;
    }

    /*
     * 89.9 达到 warning，
     * 但没有达到 critical。
     */
    if (
        check_alert_level(
            "warning level test",
            alert_evaluate_percentage(
                89.9,
                70.0,
                90.0
            ),
            ALERT_WARNING
        ) != 0
    ) {
        return 1;
    }

    /*
     * 90.0 正好达到 critical，
     * 因此应该是 CRITICAL。
     */
    if (
        check_alert_level(
            "critical boundary test",
            alert_evaluate_percentage(
                90.0,
                70.0,
                90.0
            ),
            ALERT_CRITICAL
        ) != 0
    ) {
        return 1;
    }

    if (test_alert_level_strings() != 0) {
        fprintf(
            stderr,
            "alert level string tests failed\n"
        );

        return 1;
    }

    if (test_higher_alert_level() != 0) {
        fprintf(
            stderr,
            "higher alert level tests failed\n"
        );

        return 1;
    }

    printf("all alert evaluation tests passed\n");

    return 0;
}
