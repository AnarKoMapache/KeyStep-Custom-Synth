#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 2 ]; then
  echo "Uso: $0 firmware_original.led salida_custom.led"
  exit 1
fi
ORIG="$1"
OUT="$2"
make clean
make -j"$(nproc)"
arm-none-eabi-objcopy -O binary build/KeyStep_DAC_Test.elf build/KeyStep_DAC_Test.bin
python3 make_keystep_custom_led.py "$ORIG" build/KeyStep_DAC_Test.bin "$OUT"
echo "OK: generado $OUT"
