#!/usr/bin/env bash
set -e
if [ $# -ne 2 ]; then
  echo "Uso: $0 firmware_oficial.led salida_custom.led"
  exit 1
fi
ORIG_LED="$1"
OUT_LED="$2"
make clean
make -j$(nproc)
arm-none-eabi-objcopy -O binary build/KeyStep_DAC_Test.elf build/KeyStep_DAC_Test.bin
python3 make_keystep_custom_led.py "$ORIG_LED" build/KeyStep_DAC_Test.bin "$OUT_LED"
echo "OK: generado $OUT_LED"
