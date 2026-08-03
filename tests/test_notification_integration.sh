#!/usr/bin/env bash

set -euo pipefail

if [[ "$#" -ne 2 ]]; then
    echo \
        "Usage: $0 <edgesentinel_binary> <memory_target_binary>" \
        >&2

    exit 1
fi

EDGESENTINEL_BINARY="$1"
MEMORY_TARGET_BINARY="$2"

TEMP_DIRECTORY="$(mktemp -d)"

CONFIG_FILE="$TEMP_DIRECTORY/edgesentinel.conf"
NOTIFIER_SCRIPT="$TEMP_DIRECTORY/notifier.sh"
NOTIFICATION_OUTPUT="$TEMP_DIRECTORY/notifications.log"
EDGESENTINEL_OUTPUT="$TEMP_DIRECTORY/edgesentinel-output.log"
PROGRAM_LOG="$TEMP_DIRECTORY/edgesentinel.log"
TARGET_OUTPUT="$TEMP_DIRECTORY/memory-target.log"

EDGE_PID=""
TARGET_PID=""

cleanup()
{
    if (
        [[ -n "$EDGE_PID" ]] &&
        kill -0 "$EDGE_PID" 2>/dev/null
    ); then
        kill -TERM "$EDGE_PID" 2>/dev/null || true
    fi

    if [[ -n "$EDGE_PID" ]]; then
        wait "$EDGE_PID" 2>/dev/null || true
    fi

    if (
        [[ -n "$TARGET_PID" ]] &&
        kill -0 "$TARGET_PID" 2>/dev/null
    ); then
        kill -TERM "$TARGET_PID" 2>/dev/null || true
    fi

    if [[ -n "$TARGET_PID" ]]; then
        wait "$TARGET_PID" 2>/dev/null || true
    fi

    rm -rf "$TEMP_DIRECTORY"
}

trap cleanup EXIT

cat > "$NOTIFIER_SCRIPT" <<EOF_NOTIFIER
#!/bin/sh
cat >> "$NOTIFICATION_OUTPUT"
EOF_NOTIFIER

chmod 700 "$NOTIFIER_SCRIPT"

"$MEMORY_TARGET_BINARY" \
    > "$TARGET_OUTPUT" 2>&1 &

TARGET_PID="$!"

cat > "$CONFIG_FILE" <<EOF_CONFIG
monitor_interval=1

process_pid=$TARGET_PID

process_cpu_warning_threshold=10000.0
process_cpu_critical_threshold=20000.0

process_memory_warning_threshold_mib=8.0
process_memory_critical_threshold_mib=16.0

notification_enabled=1
notification_command=$NOTIFIER_SCRIPT

log_file=$PROGRAM_LOG
log_max_size=1048576
EOF_CONFIG

"$EDGESENTINEL_BINARY" \
    -c "$CONFIG_FILE" \
    > "$EDGESENTINEL_OUTPUT" 2>&1 &

EDGE_PID="$!"

NOTIFICATION_FOUND=0

for attempt in $(seq 1 15); do
    if (
        [[ -s "$NOTIFICATION_OUTPUT" ]] &&
        grep -q \
            "PROCESS_MEMORY" \
            "$NOTIFICATION_OUTPUT" &&
        grep -Eq \
            "status=(NORMAL|WARNING)->CRITICAL" \
            "$NOTIFICATION_OUTPUT"
    ); then
        NOTIFICATION_FOUND=1
        break
    fi

    if ! kill -0 "$EDGE_PID" 2>/dev/null; then
        echo \
            "EdgeSentinel exited before notification was generated." \
            >&2

        echo "========== EdgeSentinel output ==========" >&2
        cat "$EDGESENTINEL_OUTPUT" >&2

        exit 1
    fi

    sleep 1
done

if [[ "$NOTIFICATION_FOUND" -ne 1 ]]; then
    echo \
        "Expected process memory notification was not generated." \
        >&2

    echo "========== EdgeSentinel output ==========" >&2
    cat "$EDGESENTINEL_OUTPUT" >&2

    echo "========== Memory target output ==========" >&2

    if [[ -f "$TARGET_OUTPUT" ]]; then
        cat "$TARGET_OUTPUT" >&2
    else
        echo "(memory target output does not exist)" >&2
    fi

    echo "========== Notification output ==========" >&2

    if [[ -f "$NOTIFICATION_OUTPUT" ]]; then
        cat "$NOTIFICATION_OUTPUT" >&2
    else
        echo "(notification file does not exist)" >&2
    fi

    exit 1
fi

kill -TERM "$EDGE_PID"
wait "$EDGE_PID"
EDGE_PID=""

echo "notification integration test passed"
