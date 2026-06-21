# KeyStep V29 - Mono Lead Moog + Arpegiador interno

Firmware experimental para Arturia KeyStep MK1/32 basado en la matriz real encontrada por reverse engineering.

## Hardware usado

- Select lines: PE8..PE15, activas en bajo
- Read lines: PD8..PD15, activas en bajo
- Audio: Pitch CV / DAC1 / PA4
- Mod CV / DAC2 / PA5 queda centrado

## Comportamiento

- 1 tecla presionada: mono lead Moog-ish con portamento/glide.
- 2 o más teclas presionadas: arpegiador UP interno usando las teclas mantenidas.
- El arpegiador dispara la envolvente del filtro en cada paso.

## Instalación

```bash
cd ~/Desktop/keysynth/KeyStep_DAC_Test
rm -rf keystep_keyboard_v29_mono_lead_arp
unzip ~/Descargas/keystep_keyboard_v29_mono_lead_arp.zip
chmod +x keystep_keyboard_v29_mono_lead_arp/*.sh

./keystep_keyboard_v29_mono_lead_arp/install_keyboard_v29_mono_lead_arp.sh
./keystep_keyboard_v29_mono_lead_arp/build_pack_keyboard_v29.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_v29_mono_lead_arp.led
```

## Carga

Con el KeyStep en bootloader:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_v29_mono_lead_arp.led --port hw:2,0,0
```

No conectar audífonos directo al CV. Usar entrada de sinte, mixer o interfaz con volumen bajo.
