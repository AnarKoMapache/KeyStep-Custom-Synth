#!/usr/bin/env bash
set -e

OFFICIAL_LED="${1:-keystep_Firmware_Update__1_0_28.led}"
OUT_LED="${2:-keystep_custom_keyboard_poly_v26_moogish.led}"

if [ ! -f "$OFFICIAL_LED" ]; then
  echo "ERROR: no encuentro $OFFICIAL_LED"
  exit 1
fi

if [ ! -f "make_keystep_custom_led.py" ]; then
  echo "ERROR: no encuentro make_keystep_custom_led.py"
  exit 1
fi

make clean
make -j"$(nproc)"

echo "Verificación de base:"
arm-none-eabi-objdump -h build/KeyStep_DAC_Test.elf | grep -E "isr_vector|text"

python3 make_keystep_custom_led.py "$OFFICIAL_LED" build/KeyStep_DAC_Test.bin "$OUT_LED"

echo "OK: generado $OUT_LED"
echo "Carga:"
echo "python3 keystep_rawmidi_handshake_uploader.py $OUT_LED --port hw:2,0,0"
