# KeyStep Linux MIDI Probe

Tu KeyStep MK1 en bootloader ya aparece así:

```text
ID 1c75:0208 Arturia KeyStep Updater
hw:2,0,0 KeyStep Updater MIDI 1
```

Eso confirma que el bootloader expone un puerto USB-MIDI en Linux.

## Instalar requisito

```bash
sudo apt install alsa-utils
```

## Listar puertos

```bash
python3 keystep_bootloader_probe.py --list
```

## Probar respuesta segura

Con el KeyStep en bootloader:

```bash
python3 keystep_bootloader_probe.py --identity
```

Esto envía solo el SysEx estándar MIDI Universal Device Inquiry:

```text
F0 7E 7F 06 01 F7
```

No escribe firmware.

## Inspeccionar el .led

```bash
python3 keystep_led_inspect.py keystep_Firmware_Update__1_0_28.led
```

## Qué enviar de vuelta

Pega aquí la salida completa de:

```bash
python3 keystep_bootloader_probe.py --identity
```

Según la respuesta, definimos si el bootloader acepta comandos SysEx estándar o si hay que descubrir comandos propietarios.
