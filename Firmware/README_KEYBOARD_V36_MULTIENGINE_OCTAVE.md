# KeyStep V36 MultiEngine + Octave

Firmware experimental para Arturia KeyStep original / STM32F103VCT6.

## Hardware usado

- Matriz teclado real:
  - Select: PE8..PE15, activo en bajo
  - Read: PD8..PD15, activo en bajo
- Audio:
  - DAC1 / PA4 / Pitch CV = audio mono
  - DAC2 / PA5 / Mod CV = centrado

## Motores

Mantener la tecla mas grave y tocar:

- + tecla siguiente 1: MicroFreak-ish digital
- + tecla siguiente 2: Yamaha FM
- + tecla siguiente 3: Moog lead
- + tecla siguiente 4: Acid bass
- + tecla siguiente 5: Digital bell/pad

## Octava

Mantener la tecla mas grave y tocar:

- + tecla siguiente 6: octava abajo
- + tecla siguiente 7: octava normal
- + tecla siguiente 8: octava arriba

## Instalacion

```bash
cd ~/Desktop/keysynth/KeyStep_DAC_Test
rm -rf keystep_keyboard_v36_multiengine_octave
unzip ~/Descargas/keystep_keyboard_v36_multiengine_octave.zip
chmod +x keystep_keyboard_v36_multiengine_octave/*.sh
./keystep_keyboard_v36_multiengine_octave/install_keyboard_v36_multiengine_octave.sh
./keystep_keyboard_v36_multiengine_octave/build_pack_keyboard_v36.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_v36_multiengine_octave.led
```

Luego bootloader:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_v36_multiengine_octave.led --port hw:2,0,0
```

No conectar audifonos directo al CV.
