# KeyStep Keyboard Note V23 - Real Matrix

Este firmware usa el pinout real encontrado por Ghidra:

- PE8..PE15 = selección activa en bajo
- PD8..PD15 = lectura activa en bajo
- DAC1 / PA4 / Pitch CV = audio de prueba

Diferencia con V22: V22 codificaba beeps de diagnóstico. V23 toca una sola nota continua por tecla mientras se mantiene presionada.

## Uso

Desde la raíz del proyecto:

```bash
unzip ~/Descargas/keystep_keyboard_note_v23_real_matrix.zip
chmod +x keystep_keyboard_note_v23_real_matrix/*.sh
./keystep_keyboard_note_v23_real_matrix/install_keyboard_note_v23_real_matrix.sh
./keystep_keyboard_note_v23_real_matrix/build_pack_keyboard_note_v23.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_note_v23_real_matrix.led
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_note_v23_real_matrix.led --port hw:2,0,0
```

No conectar audífonos directo al CV. Usar mixer, interfaz, entrada de sinte o Crave con volumen bajo.

Si las notas quedan invertidas de izquierda a derecha, hay que invertir la tabla `key_freq_hz[32]`.
