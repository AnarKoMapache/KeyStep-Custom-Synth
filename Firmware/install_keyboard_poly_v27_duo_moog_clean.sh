#!/usr/bin/env bash
set -e

if [ ! -f "Core/Src/main.c" ]; then
  echo "ERROR: Ejecuta esto desde la raíz del proyecto CubeMX / KeyStep_DAC_Test."
  exit 1
fi

cp Core/Src/main.c "Core/Src/main.c.backup_before_keyboard_poly_v27_$(date +%Y%m%d_%H%M%S)"
cp keystep_keyboard_poly_v27_duo_moog_clean/main_keyboard_poly_v27_duo_moog_clean.c Core/Src/main.c

echo "OK: instalado V27 DUO Moog Clean como Core/Src/main.c"
echo "V27 baja a 2 voces, sin sub grave ni desafinación, con mezcla normalizada."
echo "Matriz real: PE8-PE15 select, PD8-PD15 read. Audio: Pitch CV / DAC1 / PA4."
