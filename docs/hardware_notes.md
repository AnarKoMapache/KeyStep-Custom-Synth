# Hardware notes

## Dispositivo

- Arturia KeyStep Mk1 / KeyStep 32
- MCU observado en placa: `STM32F103VCT6`
- Arquitectura: ARM Cortex-M3
- Flash/RAM esperada en esta variante: 256 KB flash / 48 KB RAM

## Dirección base de aplicación

Durante el análisis del firmware oficial, la aplicación fue tratada con base:

```text
0x08004000
```

## Salidas DAC relevantes

```text
PA4 = DAC_OUT1 / DAC1 / Pitch CV
PA5 = DAC_OUT2 / DAC2 / Mod CV
```

En la unidad probada:

```text
Mod CV / PA5 / DAC2 = salida de audio funcional
Pitch CV / PA4 / DAC1 = no funcionó como audio útil
```

## Matriz del teclado

La matriz real fue identificada a partir de Ghidra:

```c
InputScanner8_Init(DAT_080196e4, DAT_080196e0, 8, DAT_080196dc, 8);
```

Con:

```text
DAT_080196e4 = 0x20000F44   objeto scanner
DAT_080196e0 = 0x40011800   GPIOE
DAT_080196dc = 0x40011400   GPIOD
```

Por lo tanto:

```text
PE8–PE15 = selección activa en bajo
PD8–PD15 = lectura activa en bajo
```

Esto entrega:

```text
8 selects × 8 reads = 64 contactos
32 teclas × 2 contactos = 64 contactos
```
