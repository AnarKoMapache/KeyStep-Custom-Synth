#!/usr/bin/env bash
set -e

if [ ! -d "Core/Src" ]; then
  echo "ERROR: Ejecuta esto desde la raíz del proyecto CubeMX."
  exit 1
fi

cp Core/Src/main.c "Core/Src/main.c.backup_dc_v4_$(date +%Y%m%d_%H%M%S)"
cp main_dc_v4.c Core/Src/main.c

echo "OK: instalado main_dc_v4.c como Core/Src/main.c"
