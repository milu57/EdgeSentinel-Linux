#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <stddef.h>

#include "alert_event.h"

/*
 * 通知消息缓冲区的建议长度。
 *
 * 当前通知消息包含：
 *     指标名称
 *     目标名称
 *     告警等级变化
 *     当前监测值
 *     时间
 */
#define NOTIFIER_MESSAGE_LENGTH 512

/*
 * 将告警事件格式化为可发送的通知消息。
 *
 * event：
 *     要转换的告警事件。
 *
 * buffer：
 *     用于保存生成结果的字符数组。
 *
 * buffer_size：
 *     buffer 数组的总容量。
 *
 * 成功返回 0；
 * 参数非法、时间转换失败或缓冲区不足返回 -1。
 */
int notifier_format_message(
    const AlertEvent *event,
    char *buffer,
    size_t buffer_size
);

#endif
