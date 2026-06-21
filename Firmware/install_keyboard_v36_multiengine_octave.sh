#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v36_multiengine_octave.c" Core/Src/main.c
echo "OK: instalado V36 MultiEngine + Octave como Core/Src/main.c"
echo "V36 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Motores: MicroFreak-ish, Yamaha FM, Moog lead, Acid bass, Digital bell."
echo "Modo oculto: tecla mas grave + teclas siguientes cambia motor/octava."
echo "Salida: Pitch CV / DAC1 / PA4."
