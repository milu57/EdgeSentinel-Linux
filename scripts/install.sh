#!/usr/bin/env bash

set -euo pipefail

SERVICE_NAME="edgesentinel"

SCRIPT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd
)"

PROJECT_ROOT="$(
    cd "${SCRIPT_DIR}/.."
    pwd
)"

BUILD_DIR="${PROJECT_ROOT}/build"
BINARY_FILE="${BUILD_DIR}/edgesentinel"

CONFIG_TEMPLATE="${PROJECT_ROOT}/config/edgesentinel.conf"
SERVICE_TEMPLATE="${PROJECT_ROOT}/systemd/edgesentinel.service"

INSTALL_BINARY="/usr/local/bin/edgesentinel"
SYSTEM_CONFIG_DIR="/etc/edgesentinel"
SYSTEM_CONFIG_FILE="${SYSTEM_CONFIG_DIR}/edgesentinel.conf"
LOG_DIR="/var/log/edgesentinel"
SYSTEM_SERVICE_FILE="/etc/systemd/system/edgesentinel.service"

check_command()
{
    local command_name="$1"

    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "Error: required command not found: ${command_name}"
        exit 1
    fi
}

echo "========================================"
echo " EdgeSentinel-Linux Installer"
echo "========================================"
echo "Project root: ${PROJECT_ROOT}"

#
# 整个脚本不应由 root 运行。
# 只有写入系统目录的命令单独使用 sudo。
#
if [[ "${EUID}" -eq 0 ]]; then
    echo "Error: do not run the whole installer with sudo."
    echo "Please run:"
    echo "  ./scripts/install.sh"
    exit 1
fi

echo
echo "[1/7] Checking required commands..."

check_command cmake
check_command systemctl
check_command install
check_command sudo
check_command sed

if [[ ! -f "${CONFIG_TEMPLATE}" ]]; then
    echo "Error: configuration template not found:"
    echo "  ${CONFIG_TEMPLATE}"
    exit 1
fi

if [[ ! -f "${SERVICE_TEMPLATE}" ]]; then
    echo "Error: systemd service template not found:"
    echo "  ${SERVICE_TEMPLATE}"
    exit 1
fi

echo "Required commands and project files are available."

echo
echo "[2/7] Configuring and building EdgeSentinel..."

cmake \
    -S "${PROJECT_ROOT}" \
    -B "${BUILD_DIR}"

cmake --build "${BUILD_DIR}"

if [[ ! -x "${BINARY_FILE}" ]]; then
    echo "Error: executable was not generated:"
    echo "  ${BINARY_FILE}"
    exit 1
fi

echo "Build completed successfully."

echo
echo "[3/7] Requesting administrator permission..."

sudo -v

echo
echo "[4/7] Stopping the existing service..."

if systemctl is-active --quiet "${SERVICE_NAME}"; then
    sudo systemctl stop "${SERVICE_NAME}"
    echo "Existing service stopped."
else
    echo "Service is not currently running."
fi

echo
echo "[5/7] Installing program files..."

sudo install -d -m 0755 "${SYSTEM_CONFIG_DIR}"
sudo install -d -m 0755 "${LOG_DIR}"

sudo install \
    -m 0755 \
    "${BINARY_FILE}" \
    "${INSTALL_BINARY}"

sudo install \
    -m 0644 \
    "${SERVICE_TEMPLATE}" \
    "${SYSTEM_SERVICE_FILE}"

#
# 系统配置不存在时才安装默认配置。
# 已经存在的机器配置不会被覆盖。
#
if [[ -f "${SYSTEM_CONFIG_FILE}" ]]; then
    echo "Existing configuration preserved:"
    echo "  ${SYSTEM_CONFIG_FILE}"
else
    sudo install \
        -m 0644 \
        "${CONFIG_TEMPLATE}" \
        "${SYSTEM_CONFIG_FILE}"

    #
    # 项目配置使用相对日志路径。
    # 系统服务改用 /var/log 下的绝对路径。
    #
    sudo sed -i \
        's|^[[:space:]]*log_file[[:space:]]*=.*$|log_file=/var/log/edgesentinel/edgesentinel.log|' \
        "${SYSTEM_CONFIG_FILE}"

    echo "Default system configuration installed."
fi

echo
echo "[6/7] Reloading systemd..."

sudo systemctl daemon-reload

echo
echo "[7/7] Enabling and starting EdgeSentinel..."

sudo systemctl enable --now "${SERVICE_NAME}"

if ! systemctl is-active --quiet "${SERVICE_NAME}"; then
    echo "Error: EdgeSentinel failed to start."
    echo
    sudo journalctl \
        -u "${SERVICE_NAME}" \
        -n 30 \
        --no-pager

    exit 1
fi

echo
echo "========================================"
echo " Installation completed successfully"
echo "========================================"
echo "Executable:    ${INSTALL_BINARY}"
echo "Configuration: ${SYSTEM_CONFIG_FILE}"
echo "Log directory: ${LOG_DIR}"
echo
echo "Service status:"
systemctl status "${SERVICE_NAME}" --no-pager
