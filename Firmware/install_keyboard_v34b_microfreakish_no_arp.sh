#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v34b_microfreakish_no_arp.c" Core/Src/main.c
echo "OK: instalado V34B MicroFreak-ish sin ARP como Core/Src/main.c"
echo "V34B usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Fix: sin Poly_RenderVoice duplicado; compila sobre la base V34."
echo "Sonido: digital FM/wavetable-ish mono por Pitch CV / DAC1 / PA4."
