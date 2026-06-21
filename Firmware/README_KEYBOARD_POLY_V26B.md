# KeyStep Keyboard Poly V26B Moog-ish

V26B corrige el error de compilación de V26:

- `unknown type name PolyVoice`
- `unknown type name KeyEvent`
- funciones base faltantes
- `KeyboardMatrix_ScanRows` usado antes de declararse

Sigue siendo polifonía digital de hasta 4 voces mezcladas en una sola salida:

- Select: PE8..PE15, activo en bajo
- Read: PD8..PD15, activo en bajo
- Audio: DAC1 / PA4 / Pitch CV
- DAC2 / PA5 centrado

## Uso

```bash
cd ~/Desktop/keysynth/KeyStep_DAC_Test
unzip ~/Descargas/keystep_keyboard_poly_v26b_moogish.zip
chmod +x keystep_keyboard_poly_v26b_moogish/*.sh

./keystep_keyboard_poly_v26b_moogish/install_keyboard_poly_v26b_moogish.sh
./keystep_keyboard_poly_v26b_moogish/build_pack_keyboard_poly_v26b.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_poly_v26b_moogish.led
```

Cargar:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_poly_v26b_moogish.led --port hw:2,0,0
```

No conectar audífonos directo al CV. Usar mixer, interfaz, Crave o entrada de sinte con volumen bajo.
