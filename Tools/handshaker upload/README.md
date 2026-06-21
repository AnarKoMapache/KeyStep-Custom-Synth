# KeyStep RawMIDI Handshake Uploader

Este script usa el handshake real capturado desde MIDI Control Center.

## Handshake confirmado

```text
F0 5A 57 6E 28 3C 4E 3F F7
```

Respuesta esperada:

```text
F0 3F F7
```

Se manda dos veces antes de empezar el firmware.

## Uso

Con KeyStep en bootloader:

```bash
amidi -l
```

Debe aparecer:

```text
KeyStep Updater MIDI 1
```

Extrae el script:

```bash
unzip keystep_rawmidi_handshake_uploader.zip
```

Prueba solo handshake:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_Firmware_Update__1_0_28.led --port hw:2,0,0 --only-handshake
```

Prueba handshake + primer frame oficial:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_Firmware_Update__1_0_28.led --port hw:2,0,0 --only-first-frame
```

Carga oficial completa:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_Firmware_Update__1_0_28.led --port hw:2,0,0
```

Carga custom:

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_lfo.led --port hw:2,0,0
```
