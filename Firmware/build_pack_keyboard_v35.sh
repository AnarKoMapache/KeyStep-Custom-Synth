#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 2 ]; then
  echo "Uso: $0 firmware_oficial.led salida_custom.led"
  exit 1
fi
OFFICIAL_LED="$1"
OUT_LED="$2"
make clean
make -j"$(nproc)"
arm-none-eabi-objcopy -O binary build/KeyStep_DAC_Test.elf build/KeyStep_DAC_Test.bin
python3 make_keystep_custom_led.py "$OFFICIAL_LED" build/KeyStep_DAC_Test.bin "$OUT_LED"
echo "OK: generado $OUT_LED"
