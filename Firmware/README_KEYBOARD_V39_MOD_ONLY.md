# KeyStep V39 - Mod CV Audio Only Performance

Basado en V38, pero se asume lo confirmado por prueba real:

- **Mod CV funciona como salida de audio usable**.
- **Pitch CV no importa para esta rama**.
- Para evitar confusión, el firmware escribe el audio a ambos DAC, pero la salida que debes escuchar es **Mod CV**.

## Controles ocultos

Mantén la tecla más grave y toca:

- +1 = MicroFreak-ish digital
- +2 = Yamaha FM
- +3 = Moog lead
- +4 = Acid bass
- +5 = Digital bell/pad
- +6 = octava abajo
- +7 = octava normal
- +8 = octava arriba
- +9 = timbre oscuro
- +10 = timbre normal
- +11 = timbre brillante
- +12 = vibrato off
- +13 = vibrato normal
- +14 = vibrato profundo
- +15 = hold/latch on-off

## Build

```bash
./keystep_keyboard_v39_mod_only_performance/install_keyboard_v39_mod_only_performance.sh
./keystep_keyboard_v39_mod_only_performance/build_pack_keyboard_v39.sh keystep_Firmware_Update__1_0_28.led keystep_custom_keyboard_v39_mod_only_performance.led
```

## Carga

```bash
python3 keystep_rawmidi_handshake_uploader.py keystep_custom_keyboard_v39_mod_only_performance.led --port hw:2,0,0
```

No conectar audífonos directo al CV. Usar mixer, interfaz o entrada de sinte con volumen bajo.
