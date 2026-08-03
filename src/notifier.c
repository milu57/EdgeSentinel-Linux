#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "alert.h"
#include "alert_event.h"
#include "notifier.h"

/*
 * 将告警事件格式化为统一通知文本。
 */
int notifier_format_message(
    const AlertEvent *event,
    char *buffer,
    size_t buffer_size
)
{
    struct tm local_time;
    char time_buffer[32];
    size_t formatted_time_length;
    int message_length;

    /*
     * event 是需要读取的告警事件；
     * buffer 是用于保存最终通知文本的数组。
     *
     * 任意一个为空指针，都不能继续执行。
     */
    if (
        event == NULL ||
        buffer == NULL ||
        buffer_size == 0
    ) {
        return -1;
    }

    /*
     * localtime_r() 将 time_t 时间转换成当前系统时区下的
     * 年、月、日、时、分、秒。
     *
     * 与 localtime() 不同，localtime_r() 将结果写入调用者
     * 提供的 local_time 对象，不使用共享的静态存储区。
     */
    if (
        localtime_r(
            &event->occurred_at,
            &local_time
        ) == NULL
    ) {
        return -1;
    }

    /*
     * 将时间格式化为：
     *
     *     2026-08-03 17:20:30
     */
    formatted_time_length = strftime(
        time_buffer,
        sizeof(time_buffer),
        "%Y-%m-%d %H:%M:%S",
        &local_time
    );

    if (formatted_time_length == 0) {
        return -1;
    }

    /*
     * snprintf() 最多向 buffer 中写入 buffer_size 个字节，
     * 因此不会越过数组边界。
     *
     * 返回值是不考虑 buffer 容量时，本来需要写入的字符数，
     * 不包含末尾的 '\0'。
     */
    message_length = snprintf(
        buffer,
        buffer_size,
        "[EdgeSentinel] "
        "%s target=%s status=%s->%s "
        "value=%.2f%s time=%s",
        alert_metric_to_string(event->metric),
        event->target,
        alert_level_to_string(event->previous_level),
        alert_level_to_string(event->current_level),
        event->current_value,
        event->unit,
        time_buffer
    );

    /*
     * snprintf() 返回负数，表示格式化发生错误。
     */
    if (message_length < 0) {
        return -1;
    }

    /*
     * message_length 不包含 '\0'。
     *
     * 如果它大于或等于 buffer_size，
     * 表示输出结果没有完整放入 buffer。
     */
    if ((size_t)message_length >= buffer_size) {
        return -1;
    }

    return 0;
}
