#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v35_multiengine_no_arp.c" Core/Src/main.c
echo "OK: instalado V35 MultiEngine sin ARP como Core/Src/main.c"
echo "V35 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Motores: MicroFreak-ish, Yamaha FM, Moog lead, Acid bass, Digital bell."
echo "Modo oculto: tecla mas grave + una de las 5 teclas siguientes cambia motor."
echo "Salida: Pitch CV / DAC1 / PA4."
