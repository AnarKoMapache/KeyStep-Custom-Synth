#!/usr/bin/env bash
set -e

if [ ! -f "Core/Src/main.c" ]; then
  echo "ERROR: Ejecuta esto desde la raíz del proyecto CubeMX / KeyStep_DAC_Test."
  exit 1
fi

cp Core/Src/main.c "Core/Src/main.c.backup_before_keyboard_scan_v22_$(date +%Y%m%d_%H%M%S)"
cp main_keyboard_scan_v22_real_matrix.c Core/Src/main.c

echo "OK: instalado main_keyboard_scan_v22_real_matrix.c como Core/Src/main.c"
echo "V22 usa matriz real: PE8-PE15 select, PD8-PD15 read."
echo "Audio por Pitch CV / DAC1 / PA4."
