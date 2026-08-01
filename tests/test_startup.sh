#!/usr/bin/env bash

set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 edgesentinel_executable" >&2
    exit 1
fi

edgesentinel_executable="$1"

if [ ! -x "$edgesentinel_executable" ]; then
    echo "EdgeSentinel executable not found: $edgesentinel_executable" >&2
    exit 1
fi

test_directory="$(
    mktemp -d /tmp/edgesentinel-startup-test.XXXXXX
)"

config_file="${test_directory}/startup.conf"
output_file="${test_directory}/startup-output.txt"
log_file="${test_directory}/startup.log"

process_pid=""

cleanup()
{
    if (
        [ -n "${process_pid}" ] &&
        kill -0 "${process_pid}" 2>/dev/null
    ); then
        kill -TERM "${process_pid}" 2>/dev/null || true
        wait "${process_pid}" 2>/dev/null || true
    fi

    rm -rf "${test_directory}"
}

trap cleanup EXIT

cat > "${config_file}" <<CONFIG
monitor_interval=1
process_pid=0
cpu_warning_threshold=70
cpu_critical_threshold=90
process_cpu_warning_threshold=80
process_cpu_critical_threshold=150
process_memory_warning_threshold_mib=256
process_memory_critical_threshold_mib=512
memory_warning_threshold=75
memory_critical_threshold=90
disk_warning_threshold=80
disk_critical_threshold=95
log_file=${log_file}
log_max_size=1048576
CONFIG

"${edgesentinel_executable}" \
    -c "${config_file}" \
    > "${output_file}" 2>&1 &

process_pid=$!

# 给程序留出初始化和至少一次采样的时间。
sleep 2

if ! kill -0 "${process_pid}" 2>/dev/null; then
    echo "EdgeSentinel exited before SIGINT" >&2
    cat "${output_file}" >&2

    wait "${process_pid}" 2>/dev/null || true
    process_pid=""

    exit 1
fi

# 模拟用户按下 Ctrl+C。
kill -INT "${process_pid}"

if wait "${process_pid}"; then
    run_status=0
else
    run_status=$?
fi

process_pid=""

if [ "${run_status}" -ne 0 ]; then
    echo "EdgeSentinel returned status ${run_status}" >&2
    cat "${output_file}" >&2
    exit 1
fi

if ! grep -Fq \
    "Configuration file: ${config_file}" \
    "${output_file}"
then
    echo "Configuration file output was not found" >&2
    cat "${output_file}" >&2
    exit 1
fi

if ! grep -Fq \
    "EdgeSentinel system monitor started." \
    "${output_file}"
then
    echo "Startup output was not found" >&2
    cat "${output_file}" >&2
    exit 1
fi

if ! grep -Fq \
    "EdgeSentinel stopped safely." \
    "${output_file}"
then
    echo "Safe shutdown output was not found" >&2
    cat "${output_file}" >&2
    exit 1
fi

if [ ! -f "${log_file}" ]; then
    echo "Startup log file was not created" >&2
    exit 1
fi

if ! grep -Fq \
    "EdgeSentinel started" \
    "${log_file}"
then
    echo "Startup message was not written to the log" >&2
    cat "${log_file}" >&2
    exit 1
fi

if ! grep -Fq \
    "EdgeSentinel stopped safely" \
    "${log_file}"
then
    echo "Shutdown message was not written to the log" >&2
    cat "${log_file}" >&2
    exit 1
fi

echo "startup integration test passed"
