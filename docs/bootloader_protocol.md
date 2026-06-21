# Bootloader / uploader protocol

## Entrada al bootloader

Modo observado:

```text
Mantener Record + Stop + Play/Pause mientras conectas USB
```

## Identificación USB/MIDI observada

En bootloader:

```text
amidi -l: IO hw:2,0,0 KeyStep Updater MIDI 1
lsusb: ID 1c75:0208 Arturia KeyStep Updater
```

En modo normal:

```text
amidi -l: IO hw:2,0,0 Arturia KeyStep 32 MIDI 1
```

## Handshake observado

```text
Host → KeyStep:
F0 5A 57 6E 28 3C 4E 3F F7

KeyStep → Host:
F0 3F F7
```

La secuencia de inicio se envía dos veces.

## Envío de frames

- Archivo `.led`: ASCII hex
- Frame raw observado: 2078 bytes
- Frame como ASCII hex: 4156 caracteres
- Cada payload se manda como SysEx:

```text
F0 <ASCII HEX> F7
```

ACK de frame:

```text
F0 1A F7
```

ACK final:

```text
F0 77 F7
```

## Uploader usado

```bash
python3 tools/uploader/keystep_rawmidi_handshake_uploader.py firmware.led --port hw:2,0,0
```
