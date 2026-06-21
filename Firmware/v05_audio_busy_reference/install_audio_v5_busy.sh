#!/usr/bin/env bash
set -e

if [ ! -d "Core/Src" ]; then
  echo "ERROR: Ejecuta esto desde la raíz del proyecto CubeMX."
  exit 1
fi

cp Core/Src/main.c "Core/Src/main.c.backup_audio_v5_$(date +%Y%m%d_%H%M%S)"
cp Core/Src/stm32f1xx_it.c "Core/Src/stm32f1xx_it.c.backup_audio_v5_$(date +%Y%m%d_%H%M%S)"

cp main_audio_v5_busy.c Core/Src/main.c

python3 - <<'PY'
from pathlib import Path
import re

p = Path("Core/Src/stm32f1xx_it.c")
s = p.read_text()

s = re.sub(r'\n\s*extern\s+TIM_HandleTypeDef\s+htim6\s*;\s*\n', '\n', s)

s2 = re.sub(
    r'\nvoid\s+TIM6_IRQHandler\s*\(\s*void\s*\)\s*\{.*?\n\}',
    '\n',
    s,
    flags=re.S
)

p.write_text(s2)
print("OK: removido TIM6_IRQHandler/extern htim6 si existían.")
PY

echo "OK: instalado Audio V5 Busy como Core/Src/main.c"
