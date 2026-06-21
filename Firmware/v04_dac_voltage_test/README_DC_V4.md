# KeyStep DC V4

Firmware de diagnóstico. No genera audio. Alterna voltajes lentos en DAC1/PA4 y DAC2/PA5.

## Uso

```bash
cd ~/Desktop/keysynth/KeyStep_DAC_Test
unzip ~/Descargas/keystep_dc_v4_firmware.zip
chmod +x install_dc_v4.sh build_pack_dc_v4.sh
./install_dc_v4.sh
./build_pack_dc_v4.sh keystep_Firmware_Update__1_0_28.led keystep_custom_dc_v4.led
```

Cargar en bootloader:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_dc_v4.led --port hw:2,0,0
```

## Prueba

Con multímetro:
- punta del jack = señal
- malla del jack = tierra

Debe alternar lento. Si no alterna en ninguna salida, no estamos llegando al DAC externo correcto o el firmware no corre.
