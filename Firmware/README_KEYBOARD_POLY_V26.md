# KeyStep Keyboard Poly V26 Moog-ish

Firmware experimental para Arturia KeyStep MK1/32 con matriz real de teclado:

- Select: PE8..PE15, activo en bajo
- Read: PD8..PD15, activo en bajo
- Audio: DAC1 / PA4 / Pitch CV
- DAC2 / PA5 queda centrado

## Qué cambia frente al V25

V25 era monofónico: una tecla a la vez.

V26 mezcla hasta 4 voces digitales en el mismo DAC. Es polifonía digital mono, no varias salidas CV independientes. El sonido intenta ser Moog-ish/parafónico: dos sierras, sub oscilador, filtro suave, envolvente y saturación.

## Uso

Desde la raíz del proyecto:

```bash
cd ~/Desktop/keysynth/KeyStep_DAC_Test
unzip ~/Descargas/keystep_keyboard_poly_v26_moogish.zip
chmod +x keystep_keyboard_poly_v26_moogish/*.sh

./keystep_keyboard_poly_v26_moogish/install_keyboard_poly_v26_moogish.sh
./keystep_keyboard_poly_v26_moogish/build_pack_keyboard_poly_v26.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_poly_v26_moogish.led
```

Cargar:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_poly_v26_moogish.led --port hw:2,0,0
```

No conectar audífonos directo al CV. Usar mixer, interfaz, Crave o entrada de sinte con volumen bajo.
