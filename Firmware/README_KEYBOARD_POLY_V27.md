# KeyStep Keyboard Poly V27 DUO Moog Clean

V27 busca corregir el problema de V26B: con 4 voces, sub-oscilador, sierra doble y saturación, los acordes se vuelven una masa en una sola salida DAC.

Cambios:

- 2 voces máximo en vez de 4.
- Sin sub grave audible.
- Sin desafinación entre osciladores.
- Onda más limpia: sierra suave + triángulo + pulso bajo.
- Mezcla normalizada cuando hay más de una voz.
- Release más corto.
- Sigue usando la matriz real PE/PD.

Hardware:

- Select: PE8..PE15, activo en bajo
- Read: PD8..PD15, activo en bajo
- Audio: DAC1 / PA4 / Pitch CV
- DAC2 / PA5 centrado

## Uso

```bash
cd ~/Desktop/keysynth/KeyStep_DAC_Test
rm -rf keystep_keyboard_poly_v27_duo_moog_clean
unzip ~/Descargas/keystep_keyboard_poly_v27_duo_moog_clean.zip
chmod +x keystep_keyboard_poly_v27_duo_moog_clean/*.sh

./keystep_keyboard_poly_v27_duo_moog_clean/install_keyboard_poly_v27_duo_moog_clean.sh
./keystep_keyboard_poly_v27_duo_moog_clean/build_pack_keyboard_poly_v27.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_poly_v27_duo_moog_clean.led
```

Cargar:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_poly_v27_duo_moog_clean.led --port hw:2,0,0
```

No conectar audífonos directo al CV. Usar mixer, interfaz, Crave o entrada de sinte con volumen bajo.
