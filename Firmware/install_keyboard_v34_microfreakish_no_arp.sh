#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v34_microfreakish_no_arp.c" Core/Src/main.c
echo "OK: instalado V34 MicroFreak-ish sin ARP como Core/Src/main.c"
echo "V34 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Sonido: wavetable/FM/wavefolding mono por Pitch CV / DAC1 / PA4."
