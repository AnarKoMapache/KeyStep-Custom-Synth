#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v37_performance_modcv.c" Core/Src/main.c
echo "OK: instalado V37 Performance + Mod CV como Core/Src/main.c"
echo "V37 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Motores: MicroFreak-ish, Yamaha FM, Moog lead, Acid bass, Digital bell."
echo "Agrega octava, brillo, vibrato y Mod CV/PA5 como envolvente+LFO."
echo "Salida audio: Pitch CV / DAC1 / PA4."
