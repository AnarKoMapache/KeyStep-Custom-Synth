#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v39_mod_only_performance.c" Core/Src/main.c
echo "OK: instalado V39 Mod CV Audio Only Performance como Core/Src/main.c"
echo "V39 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Audio principal: Mod CV. Pitch CV no se considera necesario; audio espejado internamente."
echo "Nuevo: hold/latch con tecla mas grave + tecla 16."
