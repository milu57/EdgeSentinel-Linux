#include <stddef.h>
#include <string.h>
#include <time.h>

#include "alert_event.h"

/*
 * 检查告警指标是否合法。
 */
static int alert_metric_is_valid(AlertMetric metric)
{
    return
        metric >= ALERT_METRIC_SYSTEM_CPU &&
        metric <= ALERT_METRIC_PROCESS_MEMORY;
}

/*
 * 检查告警等级是否合法。
 */
static int alert_level_is_valid(AlertLevel level)
{
    return
        level >= ALERT_NORMAL &&
        level <= ALERT_CRITICAL;
}

/*
 * 将监控指标转换成字符串。
 */
const char *alert_metric_to_string(AlertMetric metric)
{
    switch (metric) {
        case ALERT_METRIC_SYSTEM_CPU:
            return "SYSTEM_CPU";

        case ALERT_METRIC_SYSTEM_MEMORY:
            return "SYSTEM_MEMORY";

        case ALERT_METRIC_DISK:
            return "DISK";

        case ALERT_METRIC_PROCESS_CPU:
            return "PROCESS_CPU";

        case ALERT_METRIC_PROCESS_MEMORY:
            return "PROCESS_MEMORY";

        default:
            return "UNKNOWN";
    }
}

/*
 * 初始化一个告警事件。
 */
int alert_event_init(
    AlertEvent *event,
    AlertMetric metric,
    const char *target,
    AlertLevel previous_level,
    AlertLevel current_level,
    double current_value,
    const char *unit
)
{
    size_t target_length;
    size_t unit_length;

    /*
     * event 是要写入的告警事件对象；
     * target 和 unit 是必须读取的字符串。
     *
     * 任何一个为空指针，都不能继续处理。
     */
    if (
        event == NULL ||
        target == NULL ||
        unit == NULL
    )
    {
        return -1;
    }

    /*
     * 检查枚举值是否合法。
     */
    if (
        !alert_metric_is_valid(metric) ||
        !alert_level_is_valid(previous_level) ||
        !alert_level_is_valid(current_level)
    )
    {
        return -1;
    }

    /*
     * strlen 返回字符串实际字符数量，
     * 不包含末尾的 '\0'。
     */
    target_length = strlen(target);
    unit_length = strlen(unit);

    /*
     * 字符数组最后必须留一个位置保存 '\0'。
     *
     * 因此字符串长度不能等于或超过数组容量。
     */
    if (
        target_length >= ALERT_EVENT_TARGET_LENGTH ||
        unit_length >= ALERT_EVENT_UNIT_LENGTH
    )
    {
        return -1;
    }

    /*
     * 先把整个结构体清零。
     *
     * 这样 target 和 unit 中未使用的部分也会是 0，
     * 避免残留旧数据。
     */
    memset(event, 0, sizeof(*event));

    event->metric = metric;
    event->previous_level = previous_level;
    event->current_level = current_level;
    event->current_value = current_value;

    /*
     * 已经检查过字符串长度，因此这里复制是安全的。
     *
     * +1 表示把字符串结尾的 '\0' 一起复制。
     */
    memcpy(
        event->target,
        target,
        target_length + 1
    );

    memcpy(
        event->unit,
        unit,
        unit_length + 1
    );

    /*
     * 记录创建告警事件时的当前系统时间。
     */
    event->occurred_at = time(NULL);

    if (event->occurred_at == (time_t)-1)
    {
        return -1;
    }

    return 0;
}
