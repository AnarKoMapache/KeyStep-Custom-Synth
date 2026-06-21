#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v31_yamaha_dxish_no_arp.c" Core/Src/main.c
echo "OK: instalado V31 Yamaha DX-ish sin ARP como Core/Src/main.c"
echo "V31 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Sonido: FM mono mas Yamaha/Reface DX-ish por Pitch CV / DAC1 / PA4."
