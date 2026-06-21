# KeyStep Audio V5 Busy

Firmware audible sin TIM6 ni interrupciones. Usa el mismo acceso directo a DAC que DC V4, pero hace beeps.

## Patrón

DAC1 / Pitch CV:
- beep 220 Hz
- beep 440 Hz
- beep 880 Hz

DAC2 / Mod CV:
- alterna como marcador DC.

## Uso

```bash
cd ~/Desktop/keysynth/KeyStep_DAC_Test
unzip ~/Descargas/keystep_audio_v5_busy_firmware.zip
chmod +x install_audio_v5_busy.sh build_pack_audio_v5_busy.sh
./install_audio_v5_busy.sh
./build_pack_audio_v5_busy.sh keystep_Firmware_Update__1_0_28.led keystep_custom_audio_v5_busy.led
```

Cargar:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_audio_v5_busy.led --port hw:2,0,0
```

## Importante

No conectar audífonos directo al CV. Probar con entrada CV/audio atenuada, mixer o Crave.
