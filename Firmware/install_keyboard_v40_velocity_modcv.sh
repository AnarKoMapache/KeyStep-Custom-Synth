#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v40_velocity_modcv.c" Core/Src/main.c
echo "OK: instalado V40 Velocity/Accent Mod CV como Core/Src/main.c"
echo "V40 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Audio principal probado: Mod CV. Agrega velocity/acento por doble contacto A/B."
echo "Controles: tecla más grave + 15 hold/latch, +16 velocity OFF, +17 velocity ON."
