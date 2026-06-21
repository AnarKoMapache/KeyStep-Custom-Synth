#!/usr/bin/env bash
set -euo pipefail
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
cp "$SRC_DIR/main_keyboard_v33_rhodes_mk2_no_arp.c" Core/Src/main.c
python3 - <<'PY'
from pathlib import Path
import re
p=Path('Core/Src/stm32f1xx_it.c')
if p.exists():
    s=p.read_text()
    s=re.sub(r'\n\s*extern\s+TIM_HandleTypeDef\s+htim6\s*;\s*\n','\n',s)
    s=re.sub(r'\nvoid\s+TIM6_IRQHandler\s*\(\s*void\s*\)\s*\{.*?\n\}','\n',s,flags=re.S)
    p.write_text(s)
PY
echo "OK: instalado V33 Rhodes MK2-ish EP como Core/Src/main.c"
echo "V33 usa matriz real PE8-PE15 select, PD8-PD15 read. Sin arp. Más cuerpo/tine corto, menos campana DX."
