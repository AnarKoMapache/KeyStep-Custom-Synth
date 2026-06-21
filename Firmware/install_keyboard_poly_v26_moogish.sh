#!/usr/bin/env bash
set -e

if [ ! -f "Core/Src/main.c" ]; then
  echo "ERROR: Ejecuta esto desde la raíz del proyecto CubeMX / KeyStep_DAC_Test."
  exit 1
fi

cp Core/Src/main.c "Core/Src/main.c.backup_before_keyboard_poly_v26_$(date +%Y%m%d_%H%M%S)"
cp keystep_keyboard_poly_v26_moogish/main_keyboard_poly_v26_moogish.c Core/Src/main.c

echo "OK: instalado V26 poly como Core/Src/main.c"
echo "V26 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Sonido: 4 voces digitales Moog-ish por Pitch CV / DAC1 / PA4."
