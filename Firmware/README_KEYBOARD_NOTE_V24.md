# KeyStep Keyboard Note V24 - Soft Synth Real Matrix

Este firmware usa el pinout real encontrado por Ghidra:

- PE8..PE15 = selección activa en bajo
- PD8..PD15 = lectura activa en bajo
- DAC1 / PA4 / Pitch CV = audio de prueba

Diferencia con V23: V23 usaba onda cuadrada. V24 usa una wavetable suave con ataque y release corto para reducir el click y sonar menos áspero.

## Uso

Desde la raíz del proyecto:

```bash
unzip ~/Descargas/keystep_keyboard_note_v24_soft_synth.zip
chmod +x keystep_keyboard_note_v24_soft_synth/*.sh
./keystep_keyboard_note_v24_soft_synth/install_keyboard_note_v24_soft_synth.sh
./keystep_keyboard_note_v24_soft_synth/build_pack_keyboard_note_v24.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_note_v24_soft_synth.led
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_note_v24_soft_synth.led --port hw:2,0,0
```

No conectar audífonos directo al CV. Usar mixer, interfaz, entrada de sinte o Crave con volumen bajo.

Si las notas quedan invertidas de izquierda a derecha, invertir la tabla `key_freq_hz[32]`.
