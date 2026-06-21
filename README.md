# KeyStep USB Bootloader Protocol Kit

Objetivo: capturar cómo Arturia MIDI Control Center carga un firmware `.led` al KeyStep por USB, para luego replicar ese protocolo con un cargador propio en Python.

## Importante

Esto todavía NO carga firmware custom. Primero captura y analiza el protocolo.

Ruta de trabajo:

1. Poner KeyStep en bootloader.
2. Capturar una actualización oficial con Wireshark + USBPcap.
3. Extraer los paquetes USB OUT.
4. Compararlos contra el `.led` oficial.
5. Con esa evidencia construir el cargador Python real.

## Material

- Windows recomendado para esta etapa.
- Wireshark con USBPcap.
- Arturia MIDI Control Center.
- Firmware oficial `.led`.
- KeyStep por USB.

Wireshark puede capturar USB, y en Windows se usa USBPcap como backend de captura.

## Captura recomendada en Windows

1. Instala Wireshark y marca la opción USBPcap durante la instalación.
2. Desconecta el KeyStep.
3. Abre Wireshark.
4. Verás interfaces tipo `USBPcap1`, `USBPcap2`, etc.
5. Inicia captura en una interfaz USBPcap.
6. Pon el KeyStep en bootloader:
   - desconecta el KeyStep;
   - mantén Play + Stop + Record;
   - conecta USB sin soltar;
   - espera unos segundos.
7. Abre Arturia MIDI Control Center.
8. Carga el firmware oficial `.led`.
9. Detén la captura cuando termine.
10. Guarda como:

```text
keystep_update.pcapng
```

Si no sabes cuál interfaz USBPcap era, repite con otra o captura todas las USBPcap disponibles una por una.

## Extraer payloads con este kit

Instala `tshark` junto con Wireshark. Luego:

```bash
python extract_usb_payloads.py keystep_update.pcapng --out out_payloads
```

El script intentará extraer `usb.capdata`, `usb.data_fragment` y `usbhid.data`.

## Comparar contra el .led

```bash
python compare_led_to_capture.py keystep_Firmware_Update__1_0_28.led out_payloads
```

Qué buscamos:

- Si aparece el `.led` como texto hexadecimal ASCII.
- Si aparece el payload binario decodificado del `.led`.
- Si aparece por bloques con headers/checksum.
- Si el tráfico usa SysEx MIDI o un protocolo USB distinto.

## Archivo que debes devolver

Cuando tengas la captura, sube aquí:

```text
keystep_update.pcapng
```

o, si pesa mucho:

```text
out_payloads/
```

comprimido en ZIP.

Con eso se puede inferir el protocolo real.
