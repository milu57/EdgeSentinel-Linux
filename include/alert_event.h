#ifndef ALERT_EVENT_H
#define ALERT_EVENT_H

#include <time.h>

#include "alert.h"

/*
 * 告警目标名称的最大长度。
 *
 * 例如：
 *     system
 *     /
 *     sleep
 *     edgesentinel
 */
#define ALERT_EVENT_TARGET_LENGTH 256

/*
 * 告警数值单位的最大长度。
 *
 * 例如：
 *     %
 *     MiB
 */
#define ALERT_EVENT_UNIT_LENGTH 16

/*
 * 告警事件对应的监控指标。
 */
typedef enum {
    ALERT_METRIC_SYSTEM_CPU = 0,
    ALERT_METRIC_SYSTEM_MEMORY,
    ALERT_METRIC_DISK,
    ALERT_METRIC_PROCESS_CPU,
    ALERT_METRIC_PROCESS_MEMORY
} AlertMetric;

/*
 * 一次完整的告警等级变化事件。
 *
 * 例如：
 *
 *     metric         = ALERT_METRIC_SYSTEM_CPU
 *     target         = "system"
 *     previous_level = ALERT_NORMAL
 *     current_level  = ALERT_WARNING
 *     current_value  = 75.30
 *     unit           = "%"
 *     occurred_at    = 告警发生时间
 */
typedef struct {
    /*
     * 哪一种监控指标发生了告警变化。
     */
    AlertMetric metric;

    /*
     * 告警所对应的具体目标。
     *
     * 系统 CPU 或系统内存可使用 "system"；
     * 磁盘可使用挂载路径 "/"；
     * 进程告警可使用进程名称。
     */
    char target[ALERT_EVENT_TARGET_LENGTH];

    /*
     * 变化前的告警等级。
     */
    AlertLevel previous_level;

    /*
     * 变化后的告警等级。
     */
    AlertLevel current_level;

    /*
     * 发生告警变化时监测到的实际数值。
     */
    double current_value;

    /*
     * 数值的单位。
     */
    char unit[ALERT_EVENT_UNIT_LENGTH];

    /*
     * 告警事件发生的时间。
     *
     * time_t 是 C 标准库表示时间的类型，
     * 后续可以转换成可读的日期和时间。
     */
    time_t occurred_at;
} AlertEvent;

/*
 * 将监控指标转换成字符串。
 */
const char *alert_metric_to_string(AlertMetric metric);

/*
 * 初始化一个告警事件。
 *
 * 成功返回 0；
 * 参数非法或字符串过长返回 -1。
 */
int alert_event_init(
    AlertEvent *event,
    AlertMetric metric,
    const char *target,
    AlertLevel previous_level,
    AlertLevel current_level,
    double current_value,
    const char *unit
);

#endif
