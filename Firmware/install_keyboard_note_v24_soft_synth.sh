#!/usr/bin/env bash
set -e

if [ ! -f "Core/Src/main.c" ]; then
  echo "ERROR: Ejecuta esto desde la raíz del proyecto CubeMX / KeyStep_DAC_Test."
  exit 1
fi

cp Core/Src/main.c "Core/Src/main.c.backup_before_keyboard_note_v24_$(date +%Y%m%d_%H%M%S)"
cp keystep_keyboard_note_v24_soft_synth/main_keyboard_note_v24_soft_synth.c Core/Src/main.c

echo "OK: instalado V24 como Core/Src/main.c"
echo "V24 usa matriz real: PE8-PE15 select, PD8-PD15 read."
echo "Cada tecla toca un tono wavetable mas suave por Pitch CV / DAC1 / PA4."
