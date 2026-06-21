#!/usr/bin/env bash
set -e
cp keystep_keyboard_v30_reface_dxish_fm_arp/main_keyboard_v30_reface_dxish_fm_arp.c Core/Src/main.c
printf 'OK: instalado V30 Reface DX-ish FM Arp como Core/Src/main.c
'
printf 'V30 usa matriz real PE8-PE15 select, PD8-PD15 read.
'
printf 'Sonido: mono FM digital tipo Reface DX-ish + portamento + arpegiador por Pitch CV / DAC1 / PA4.
'
