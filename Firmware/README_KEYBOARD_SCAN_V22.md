# KeyStep Keyboard Scan V22 - matriz real PE/PD

Este firmware ya no hace mapper a ciegas. Usa el pinout encontrado en Ghidra:

- **PE8–PE15**: líneas de selección, activas en bajo.
- **PD8–PD15**: líneas de lectura, activas en bajo.
- 8 selects × 8 reads = 64 contactos.
- 32 teclas × 2 contactos = 64 contactos.

## Matriz

- PE8  = contacto A grupo 0, lee PD8–PD15
- PE9  = contacto B grupo 0, lee PD8–PD15
- PE10 = contacto A grupo 1, lee PD8–PD15
- PE11 = contacto B grupo 1, lee PD8–PD15
- PE12 = contacto A grupo 2, lee PD8–PD15
- PE13 = contacto B grupo 2, lee PD8–PD15
- PE14 = contacto A grupo 3, lee PD8–PD15
- PE15 = contacto B grupo 3, lee PD8–PD15

Para `keyIndex` 0..31:

```text
grupo   = keyIndex / 8
columna = keyIndex % 8

contacto A: PE = 8 + grupo*2,     PD = 8 + columna
contacto B: PE = 8 + grupo*2 + 1, PD = 8 + columna
```

## Qué hace el firmware

- Escanea la matriz real.
- Si detecta tecla, suena por **Pitch CV / DAC1 / PA4**.
- También codifica por beeps:
  - Contacto A = 1 beep.
  - Contacto B = 2 beeps.
  - A+B = 3 beeps.
  - Luego keyIndex: decenas en beeps graves, unidades+1 en beeps agudos.
- Espera soltar la tecla antes de reportar otra.

## Uso

```bash
cd ~/Desktop/keysynth/KeyStep_DAC_Test
unzip ~/Descargas/keystep_keyboard_scan_v22_real_matrix.zip
chmod +x install_keyboard_scan_v22_real_matrix.sh build_pack_keyboard_scan_v22.sh
./install_keyboard_scan_v22_real_matrix.sh
./build_pack_keyboard_scan_v22.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_scan_v22_real_matrix.led
```

Cargar en bootloader:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_scan_v22_real_matrix.led --port hw:2,0,0
```

## Escucha

Escucha **Pitch CV / DAC1 / PA4** mediante sinte, mixer o interfaz, con volumen bajo.

No conectar audífonos directo al CV.

## Resultado esperado

Al arrancar:

- tres beeps de inicio
- tres beeps de listo

Al presionar una tecla:

- beep de inicio de evento
- código de contacto
- código de keyIndex
- tono principal según la tecla

Si no suena al presionar teclas, avisar si al menos hace los beeps de arranque.
