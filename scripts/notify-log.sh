#!/bin/sh

#
# EdgeSentinel 示例通知脚本。
#
# EdgeSentinel 会把一条格式化后的告警消息
# 通过标准输入传递给本脚本。
#
# 默认保存位置：
#     logs/notifications.log
#
# 也可以通过环境变量覆盖：
#     EDGESENTINEL_NOTIFICATION_LOG=/path/to/file
#

set -eu

SCRIPT_DIRECTORY=$(
    CDPATH= cd -- "$(dirname -- "$0")" &&
    pwd
)

PROJECT_ROOT=$(
    dirname "$SCRIPT_DIRECTORY"
)

NOTIFICATION_LOG="${EDGESENTINEL_NOTIFICATION_LOG:-$PROJECT_ROOT/logs/notifications.log}"

NOTIFICATION_DIRECTORY=$(
    dirname "$NOTIFICATION_LOG"
)

mkdir -p "$NOTIFICATION_DIRECTORY"

cat >> "$NOTIFICATION_LOG"
