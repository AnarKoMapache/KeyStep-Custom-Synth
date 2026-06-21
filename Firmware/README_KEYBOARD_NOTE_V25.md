# KeyStep Keyboard Note V25 - Moogish Synth Real Matrix

Este firmware usa el pinout real encontrado por Ghidra:

- PE8..PE15 = selección activa en bajo
- PD8..PD15 = lectura activa en bajo
- DAC1 / PA4 / Pitch CV = audio de prueba

Diferencia con V24: V25 usa dos osciladores tipo sierra, sub oscilador, filtro pasa-bajos con envolvente y saturación suave para acercarse más a un monosinte tipo Moog.

## Uso

Desde la raíz del proyecto:

```bash
unzip ~/Descargas/keystep_keyboard_note_v25_moogish_synth.zip
chmod +x keystep_keyboard_note_v25_moogish_synth/*.sh
./keystep_keyboard_note_v25_moogish_synth/install_keyboard_note_v25_moogish_synth.sh
./keystep_keyboard_note_v25_moogish_synth/build_pack_keyboard_note_v25.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_note_v25_moogish_synth.led
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_note_v25_moogish_synth.led --port hw:2,0,0
```

No conectar audífonos directo al CV. Usar mixer, interfaz, entrada de sinte o Crave con volumen bajo.

Si queda muy grave o muy oscuro, se puede subir la tabla de notas o abrir más el filtro en `moog_render_sample()`.
