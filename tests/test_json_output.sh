#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 edgesentinel_binary" >&2
    exit 1
fi

edgesentinel_binary="$1"

temporary_directory="$(mktemp -d)"

cleanup()
{
    rm -rf "${temporary_directory}"
}

trap cleanup EXIT

configuration_file="${temporary_directory}/json-output.conf"
json_output_file="${temporary_directory}/output.jsonl"
status_output_file="${temporary_directory}/status.log"

cat > "${configuration_file}" <<'EOF'
monitor_interval=1
EOF

# timeout 正常到期时返回 124。
# 对持续运行的监控程序来说，这是预期行为。
set +e

timeout 4s \
    "${edgesentinel_binary}" \
    -c "${configuration_file}" \
    --output json \
    > "${json_output_file}" \
    2> "${status_output_file}"

program_status=$?

set -e

if [[ ${program_status} -ne 0 && ${program_status} -ne 124 ]]
then
    echo \
        "EdgeSentinel exited unexpectedly: ${program_status}" \
        >&2

    cat "${status_output_file}" >&2
    exit 1
fi

if [[ ! -s "${json_output_file}" ]]
then
    echo "JSON output file is empty." >&2
    exit 1
fi

python3 - "${json_output_file}" <<'PY'
import json
import sys

path = sys.argv[1]

with open(path, "r", encoding="utf-8") as file:
    lines = [line.strip() for line in file if line.strip()]

if not lines:
    raise SystemExit("No JSON Lines were produced.")

for line_number, line in enumerate(lines, start=1):
    try:
        data = json.loads(line)
    except json.JSONDecodeError as error:
        raise SystemExit(
            f"Invalid JSON on line {line_number}: {error}"
        ) from error

    required_fields = {
        "timestamp",
        "uptime",
        "load_average",
        "system",
        "network",
        "processes",
    }

    missing_fields = required_fields.difference(data)

    if missing_fields:
        raise SystemExit(
            f"Line {line_number} is missing fields: "
            f"{sorted(missing_fields)}"
        )

print(f"Validated {len(lines)} JSON line(s).")
PY

if ! grep -q \
    "EdgeSentinel system monitor started." \
    "${status_output_file}"
then
    echo "Startup status was not written to stderr." >&2
    exit 1
fi

echo "JSON output integration test passed."
