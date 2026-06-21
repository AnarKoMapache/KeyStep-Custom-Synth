#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/main_keyboard_v38_audio_mod_pitchcv.c" Core/Src/main.c
echo "OK: instalado V38 Audio Mod + PitchCV auxiliar como Core/Src/main.c"
echo "V38 usa matriz real PE8-PE15 select, PD8-PD15 read."
echo "Audio: queda en la salida fisica que funciono en V37: Mod CV."
echo "Aux: el otro DAC entrega CV de nota aproximado, no audio."
