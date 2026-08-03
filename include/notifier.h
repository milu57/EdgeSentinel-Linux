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

/*
 * 执行外部通知程序并发送告警事件。
 *
 * event：
 *     要发送的告警事件。
 *
 * command：
 *     外部通知程序或脚本的路径。
 *
 * 通知消息通过外部程序的标准输入传递。
 *
 * 成功返回 0；
 * 参数非法、进程创建失败、写入失败或外部程序
 * 返回非零状态时返回 -1。
 */
int notifier_send(
    const AlertEvent *event,
    const char *command
);

#endif
