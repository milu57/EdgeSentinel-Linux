#ifndef ALERT_H
#define ALERT_H

/*
 * 系统资源告警等级
 */
typedef enum {
    ALERT_NORMAL = 0,
    ALERT_WARNING,
    ALERT_CRITICAL
} AlertLevel;

/*
 * 将告警等级转换成字符串
 */
const char *alert_level_to_string(AlertLevel level);

/*
 * 根据当前数值和阈值判断告警等级
 */
AlertLevel alert_evaluate_percentage(
    double value,
    double warning_threshold,
    double critical_threshold
);

/*
 * 比较两个告警等级，返回更严重的等级
 */
AlertLevel alert_get_higher_level(
    AlertLevel first,
    AlertLevel second
);

#endif
