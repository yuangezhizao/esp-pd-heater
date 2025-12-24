#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
OUT_DIR="${2:-dist}"
CHIP="${IDF_TARGET:-esp32c3}"

FLASH_PROJECT_ARGS="${BUILD_DIR}/flash_project_args"
if [[ ! -f "${FLASH_PROJECT_ARGS}" ]]; then
  echo "Missing ${FLASH_PROJECT_ARGS} (run: idf.py build)" >&2
  exit 1
fi

mkdir -p "${OUT_DIR}"

BOOTLOADER_BIN="${BUILD_DIR}/bootloader/bootloader.bin"
PARTITION_BIN="${BUILD_DIR}/partition_table/partition-table.bin"
APP_BIN_GLOB=("${BUILD_DIR}"/*.bin)

if [[ -f "${BOOTLOADER_BIN}" ]]; then
  cp -f "${BOOTLOADER_BIN}" "${OUT_DIR}/bootloader.bin"
fi
if [[ -f "${PARTITION_BIN}" ]]; then
  cp -f "${PARTITION_BIN}" "${OUT_DIR}/partition-table.bin"
fi

# Copy the app binary referenced in flash_project_args (skip bootloader/partition_table)
APP_BIN_REL="$(awk 'NR>1 && $2 ~ /\.bin$/ && index($2,"bootloader/")!=1 && index($2,"partition_table/")!=1 {print $2; exit}' "${FLASH_PROJECT_ARGS}")"
APP_BIN="${BUILD_DIR}/${APP_BIN_REL}"
if [[ -f "${APP_BIN}" ]]; then
  cp -f "${APP_BIN}" "${OUT_DIR}/$(basename "${APP_BIN}")"
else
  # Fallback: copy any top-level .bin in build dir
  for f in "${APP_BIN_GLOB[@]}"; do
    [[ -f "${f}" ]] || continue
    cp -f "${f}" "${OUT_DIR}/$(basename "${f}")"
  done
fi

cp -f "${FLASH_PROJECT_ARGS}" "${OUT_DIR}/flash_project_args.orig"
if [[ -f "${BUILD_DIR}/flasher_args.json" ]]; then
  cp -f "${BUILD_DIR}/flasher_args.json" "${OUT_DIR}/flasher_args.json"
fi

# Create a dist-local flash args file that references files in OUT_DIR.
{
  head -n1 "${FLASH_PROJECT_ARGS}"
  awk '
    NR==1 {next}
    {
      off=$1
      path=$2
      if (path ~ /^bootloader\//) print off, "bootloader.bin"
      else if (path ~ /^partition_table\//) print off, "partition-table.bin"
      else {
        n=split(path, parts, "/")
        print off, parts[n]
      }
    }
  ' "${FLASH_PROJECT_ARGS}"
} > "${OUT_DIR}/flash_project_args"

# Best-effort merged firmware (optional; requires esptool in the environment).
if python -c 'import esptool' >/dev/null 2>&1; then
  python - "${BUILD_DIR}" "${OUT_DIR}" "${CHIP}" <<'PY'
import os
import shlex
import subprocess
from pathlib import Path
import sys

build_dir = Path(sys.argv[1])
out_dir = Path(sys.argv[2])
chip = sys.argv[3]
flash_args_path = build_dir / "flash_project_args"

lines = flash_args_path.read_text(encoding="utf-8").splitlines()
flash_opts = shlex.split(lines[0]) if lines else []
pairs = []
for line in lines[1:]:
    parts = line.split()
    if len(parts) < 2:
        continue
    off, rel = parts[0], parts[1]
    pairs.extend([off, str(build_dir / rel)])

cmd = ["python", "-m", "esptool", "--chip", chip, "merge_bin", "-o", str(out_dir / "merged-firmware.bin")]
cmd.extend(flash_opts)
cmd.extend(pairs)
subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
PY
fi

(
  cd "${OUT_DIR}"
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum * > SHA256SUMS.txt
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 * > SHA256SUMS.txt
  fi
)
