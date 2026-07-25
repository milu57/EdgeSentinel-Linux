#!/usr/bin/env bash

set -euo pipefail

SERVICE_NAME="edgesentinel"

INSTALL_BINARY="/usr/local/bin/edgesentinel"
SYSTEM_SERVICE_FILE="/etc/systemd/system/edgesentinel.service"

SYSTEM_CONFIG_DIR="/etc/edgesentinel"
LOG_DIR="/var/log/edgesentinel"

check_command()
{
    local command_name="$1"

    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "Error: required command not found: ${command_name}"
        exit 1
    fi
}

echo "========================================"
echo " EdgeSentinel-Linux Uninstaller"
echo "========================================"

#
# 整个脚本不要直接使用 sudo 运行。
# 需要管理员权限的命令会在脚本内部单独调用 sudo。
#
if [[ "${EUID}" -eq 0 ]]; then
    echo "Error: do not run the whole uninstaller with sudo."
    echo "Please run:"
    echo "  ./scripts/uninstall.sh"
    exit 1
fi

echo
echo "[1/5] Checking required commands..."

check_command sudo
check_command systemctl
check_command rm

echo "Required commands are available."

echo
echo "[2/5] Requesting administrator permission..."

sudo -v

echo
echo "[3/5] Stopping and disabling EdgeSentinel..."

if systemctl is-active --quiet "${SERVICE_NAME}"; then
    sudo systemctl stop "${SERVICE_NAME}"
    echo "Service stopped."
else
    echo "Service is not currently running."
fi

if systemctl is-enabled --quiet "${SERVICE_NAME}" 2>/dev/null; then
    sudo systemctl disable "${SERVICE_NAME}"
    echo "Service disabled."
else
    echo "Service is not enabled."
fi

echo
echo "[4/5] Removing installed program files..."

if [[ -f "${INSTALL_BINARY}" ]]; then
    sudo rm -f "${INSTALL_BINARY}"
    echo "Removed executable:"
    echo "  ${INSTALL_BINARY}"
else
    echo "Executable was not found:"
    echo "  ${INSTALL_BINARY}"
fi

if [[ -f "${SYSTEM_SERVICE_FILE}" ]]; then
    sudo rm -f "${SYSTEM_SERVICE_FILE}"
    echo "Removed systemd service file:"
    echo "  ${SYSTEM_SERVICE_FILE}"
else
    echo "Systemd service file was not found:"
    echo "  ${SYSTEM_SERVICE_FILE}"
fi

echo
echo "[5/5] Reloading systemd..."

sudo systemctl daemon-reload
sudo systemctl reset-failed "${SERVICE_NAME}" 2>/dev/null || true

echo
echo "========================================"
echo " Uninstallation completed successfully"
echo "========================================"
echo
echo "Preserved configuration:"
echo "  ${SYSTEM_CONFIG_DIR}"
echo
echo "Preserved logs:"
echo "  ${LOG_DIR}"
echo
echo "To reinstall EdgeSentinel, run:"
echo "  ./scripts/install.sh"
