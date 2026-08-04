#!/usr/bin/env bash

#
# 在 x86_64 Linux 主机上交叉编译 EdgeSentinel，
# 并通过 QEMU 验证 ARM64 版本的全部 13 项测试。
#

set -euo pipefail

PROJECT_ROOT="$(
    cd "$(dirname "${BASH_SOURCE[0]}")/.."
    pwd
)"

BUILD_DIR="${PROJECT_ROOT}/build-aarch64"

TOOLCHAIN_FILE="$(
    realpath \
        "${PROJECT_ROOT}/cmake/toolchains/aarch64-linux-gnu.cmake"
)"

QEMU_SYSROOT="/usr/aarch64-linux-gnu"

TEMP_DIRECTORY="$(
    mktemp -d /tmp/edgesentinel-arm64-qemu.XXXXXX
)"

BUILD_LOG="${TEMP_DIRECTORY}/build.log"

EDGE_WRAPPER="${TEMP_DIRECTORY}/edgesentinel"
MEMORY_TARGET_WRAPPER="${TEMP_DIRECTORY}/notification_memory_target"

TOTAL_TESTS=13
PASSED_TESTS=0

cleanup()
{
    rm -rf "${TEMP_DIRECTORY}"
}

trap cleanup EXIT

require_command()
{
    local command_name="$1"

    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo \
            "Required command not found: ${command_name}" \
            >&2

        exit 1
    fi
}

echo "=========================================="
echo " EdgeSentinel ARM64 QEMU verification"
echo "=========================================="

for command_name in \
    aarch64-linux-gnu-gcc \
    bash \
    cmake \
    file \
    grep \
    python3 \
    qemu-aarch64 \
    realpath \
    timeout
do
    require_command "${command_name}"
done

QEMU_BINARY="$(command -v qemu-aarch64)"

if [[ ! -f "${TOOLCHAIN_FILE}" ]]; then
    echo \
        "ARM64 toolchain file not found: ${TOOLCHAIN_FILE}" \
        >&2

    exit 1
fi

if [[ ! -d "${QEMU_SYSROOT}" ]]; then
    echo \
        "ARM64 sysroot not found: ${QEMU_SYSROOT}" \
        >&2

    exit 1
fi

echo
echo "[Build] Removing the previous ARM64 build directory..."

rm -rf "${BUILD_DIR}"

echo
echo "[Build] Configuring ARM64 Release build..."

cmake \
    -S "${PROJECT_ROOT}" \
    -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DCMAKE_BUILD_TYPE=Release

echo
echo "[Build] Building all ARM64 targets..."

LC_ALL=C \
cmake \
    --build "${BUILD_DIR}" \
    2>&1 |
tee "${BUILD_LOG}"

if grep -Ein "warning:" "${BUILD_LOG}"
then
    echo "ARM64 build contains compiler warnings." >&2
    exit 1
fi

echo
echo "[Build] Checking executable architecture..."

file "${BUILD_DIR}/edgesentinel"

if ! file "${BUILD_DIR}/edgesentinel" |
    grep -q "ARM aarch64"
then
    echo "edgesentinel is not an ARM64 executable." >&2
    exit 1
fi

if ! file "${BUILD_DIR}/notification_memory_target" |
    grep -q "ARM aarch64"
then
    echo \
        "notification_memory_target is not an ARM64 executable." \
        >&2

    exit 1
fi

echo
echo "[Wrapper] Creating QEMU execution wrappers..."

cat > "${EDGE_WRAPPER}" <<EOF
#!/usr/bin/env bash

exec "${QEMU_BINARY}" \
    -L "${QEMU_SYSROOT}" \
    "${BUILD_DIR}/edgesentinel" \
    "\$@"
EOF

cat > "${MEMORY_TARGET_WRAPPER}" <<EOF
#!/usr/bin/env bash

exec "${QEMU_BINARY}" \
    -L "${QEMU_SYSROOT}" \
    "${BUILD_DIR}/notification_memory_target" \
    "\$@"
EOF

chmod 700 \
    "${EDGE_WRAPPER}" \
    "${MEMORY_TARGET_WRAPPER}"

run_arm64_program_test()
{
    local test_name="$1"
    local executable_name="$2"

    echo
    echo "[$((PASSED_TESTS + 1))/${TOTAL_TESTS}] ${test_name}"

    "${QEMU_BINARY}" \
        -L "${QEMU_SYSROOT}" \
        "${BUILD_DIR}/${executable_name}"

    PASSED_TESTS=$((PASSED_TESTS + 1))
}

run_integration_test()
{
    local test_name="$1"
    shift

    echo
    echo "[$((PASSED_TESTS + 1))/${TOTAL_TESTS}] ${test_name}"

    bash "$@"

    PASSED_TESTS=$((PASSED_TESTS + 1))
}

#
# 9 个编译生成的 C 测试程序。
#

run_arm64_program_test \
    "process_monitor_test" \
    "test_process_monitor"

run_arm64_program_test \
    "config_test" \
    "test_config"

run_arm64_program_test \
    "alert_test" \
    "test_alert"

run_arm64_program_test \
    "logger_test" \
    "test_logger"

run_arm64_program_test \
    "system_resources_test" \
    "test_system_resources"

run_arm64_program_test \
    "calculations_test" \
    "test_calculations"

run_arm64_program_test \
    "output_test" \
    "test_output"

run_arm64_program_test \
    "alert_event_test" \
    "test_alert_event"

run_arm64_program_test \
    "notifier_test" \
    "test_notifier"

#
# 4 个原有 Bash 集成测试。
#

run_integration_test \
    "startup_integration_test" \
    "${PROJECT_ROOT}/tests/test_startup.sh" \
    "${EDGE_WRAPPER}"

run_integration_test \
    "invalid_config_startup_test" \
    "${PROJECT_ROOT}/tests/test_invalid_config_startup.sh" \
    "${EDGE_WRAPPER}"

run_integration_test \
    "json_output_integration_test" \
    "${PROJECT_ROOT}/tests/test_json_output.sh" \
    "${EDGE_WRAPPER}"

run_integration_test \
    "notification_integration_test" \
    "${PROJECT_ROOT}/tests/test_notification_integration.sh" \
    "${EDGE_WRAPPER}" \
    "${MEMORY_TARGET_WRAPPER}"

echo
echo "=========================================="
echo " ARM64 QEMU tests passed: ${PASSED_TESTS}/${TOTAL_TESTS}"
echo "=========================================="
