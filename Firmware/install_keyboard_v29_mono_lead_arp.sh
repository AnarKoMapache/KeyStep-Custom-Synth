#!/usr/bin/env bash
set -e

if [ ! -f "Core/Src/main.c" ]; then
  echo "ERROR: Ejecuta esto desde la raíz del proyecto CubeMX / KeyStep_DAC_Test."
  exit 1
fi

cp Core/Src/main.c "Core/Src/main.c.backup_before_keyboard_v29_mono_lead_arp_$(date +%Y%m%d_%H%M%S)"
cp keystep_keyboard_v29_mono_lead_arp/main_keyboard_v29_mono_lead_arp.c Core/Src/main.c

echo "OK: instalado V29 Mono Lead Moog + Arpegiador como Core/Src/main.c"
echo "Matriz real: PE8-PE15 select, PD8-PD15 read."
echo "Modo: 1 tecla = mono lead con portamento; 2+ teclas = arpegiador UP interno."
echo "Audio: Pitch CV / DAC1 / PA4."
