# KeyFreak 🎹

**Convertí un Arturia KeyStep Mk1 en un mini sintetizador digital que toca por sí solo.**

Este repo documenta cómo lo hice: desde abrir el equipo y encontrar el microcontrolador, hasta capturar el protocolo de carga, descubrir cómo está cableado el teclado y escribir firmware propio que genera audio. Es un proyecto experimental y educativo, hecho a punta de prueba y error. Si te gusta el firmware embebido, la ingeniería inversa o el DIY de sintetizadores, espero que te sirva.

> **Estado:** prueba de concepto funcional. El KeyStep arranca firmware propio, lee sus 32 teclas y suena por la salida Mod CV. No es un producto terminado ni reemplaza al firmware oficial.

- **Microcontrolador:** STM32F103VCT6 (ARM Cortex-M3)
- **Cargador custom:** `KS-BootBridge`
- **Salida de audio que funciona:** Mod CV / DAC2 / PA5
- **Herramientas:** Ghidra · Wireshark/USBPcap · Python · GCC ARM Embedded · raw MIDI/SysEx
- **Autor:** Mapache Bastardx

---

## ⚠️ Antes de empezar: léete esto

Modificar el firmware de un equipo comercial **puede dejarlo inservible**. Todo lo que está acá es experimental y lo hago bajo mi propio riesgo; si lo replicas, también es bajo el tuyo.

**Eléctricamente:**
- No conectes audífonos directo a las salidas CV.
- Usa una mixer, interfaz de audio o entrada de sinte con el volumen bajo.
- No sueldes modificaciones permanentes sin verificar pines y voltajes.
- No alimentes módulos externos desde puntos internos sin medir antes.

**Y por respeto a Arturia y a la legalidad**, en este repo (y te pido lo mismo si haces un fork):
- No subo firmware oficial `.led`, ni dumps binarios, ni nada propietario extraído del fabricante.
- No clono ni reemplazo comercialmente nada de Arturia, ni afirmo afiliación con ellos.
- Si alguna herramienta necesita el firmware oficial, el usuario aporta el suyo **localmente**; no se redistribuye.

Las marcas comerciales se usan solo de forma descriptiva.

---

## Tabla de contenidos

1. [La pregunta de la que salió todo](#1-la-pregunta-de-la-que-salió-todo)
2. [El hardware por dentro](#2-el-hardware-por-dentro)
3. [Entrar en modo bootloader](#3-entrar-en-modo-bootloader)
4. [Capturar el protocolo con Wireshark](#4-capturar-el-protocolo-con-wireshark)
5. [El formato `.led` y el cargador KS-BootBridge](#5-el-formato-led-y-el-cargador-ks-bootbridge)
6. [Lo que encontré en Ghidra](#6-lo-que-encontré-en-ghidra)
7. [El gran descubrimiento: la matriz del teclado](#7-el-gran-descubrimiento-la-matriz-del-teclado)
8. [Primer audio y primeras teclas](#8-primer-audio-y-primeras-teclas)
9. [Probando motores de síntesis](#9-probando-motores-de-síntesis)
10. [MultiEngine y controles ocultos](#10-multiengine-y-controles-ocultos)
11. [Mod CV como salida principal](#11-mod-cv-como-salida-principal)
12. [Velocity por doble contacto](#12-velocity-por-doble-contacto)
13. [Tabla de versiones](#13-tabla-de-versiones)
14. [Arquitectura final del firmware](#14-arquitectura-final-del-firmware)
15. [Comparación honesta con sintes comerciales](#15-comparación-honesta-con-sintes-comerciales)
16. [Trabajo futuro](#16-trabajo-futuro)
17. [Estructura del repo](#17-estructura-del-repo)
18. [Cierre](#18-cierre)

---

## 1. La pregunta de la que salió todo

Todo empezó con una duda bastante simple:

> ¿Puede un Arturia KeyStep Mk1 ejecutar mi propio firmware y generar sonido por sí mismo, en vez de ser solo un controlador?

El KeyStep nació como controlador MIDI/CV, pero por dentro lleva un microcontrolador STM32 perfectamente capaz de correr código propio, leer un teclado, mover GPIO, usar sus DACs y generar audio. Esa pregunta se convirtió en una lista de objetivos: identificar el chip, entender el bootloader y el formato de actualización, cargar código propio sin tocar herramientas oficiales, leer el teclado físico, sacar audio del DAC interno, armar motores de síntesis simples y dejarlo todo documentado para que otros puedan seguir el camino.

Lo que sigue es ese recorrido, más o menos en el orden en que las cosas fueron encajando (con sus tropiezos incluidos, porque ahí está lo interesante).

---

## 2. El hardware por dentro

Abriendo el equipo e inspeccionando la placa, el cerebro resultó ser un **STM32F103VCT6**:

| Característica | Valor aproximado |
|---|---|
| Núcleo | ARM Cortex-M3 |
| Frecuencia | hasta 72 MHz |
| Flash | ~256 KB |
| RAM | ~48 KB |
| DAC | 2 canales de 12 bits |

Ese DAC de dos canales es la clave para hacer ruido. Sus salidas son:

```text
PA4 = DAC_OUT1 = DAC1
PA5 = DAC_OUT2 = DAC2
```

Al principio asumí que el audio saldría por `Pitch CV / DAC1 / PA4`. **Estaba equivocado.** En la unidad que probé, esa salida no servía como audio útil, y la que terminó funcionando fue:

```text
Audio principal = Mod CV / DAC2 / PA5
```

Lo aclaro acá arriba porque me costó iteraciones darme cuenta, y no quiero que pierdas tiempo con la misma hipótesis. Más detalle en la [sección 11](#11-mod-cv-como-salida-principal).

---

## 3. Entrar en modo bootloader

El KeyStep entra en modo de actualización si mantienes una combinación de botones mientras lo conectas por USB:

```text
Record + Stop + Play/Pause + conectar USB
```

En Linux aparece como un dispositivo distinto:

```text
USB:  Arturia KeyStep Updater
MIDI: KeyStep Updater MIDI 1
```

En ese modo, el equipo escucha firmware por mensajes MIDI/SysEx. El problema era: ¿cómo se los envío yo?

---

## 4. Capturar el protocolo con Wireshark

Para cargar firmware propio necesitaba entender cómo el updater oficial le habla al KeyStep. Así que capturé el tráfico de una actualización oficial con **Wireshark + USBPcap** y observé el flujo:

```text
PC / updater
   ↓  handshake SysEx
KeyStep responde
   ↓  frames de firmware
ACK por cada frame
   ↓
ACK final
```

Los mensajes que identifiqué fueron:

| Mensaje | Bytes |
|---|---|
| Handshake (PC → KeyStep) | `F0 5A 57 6E 28 3C 4E 3F F7` |
| Respuesta del KeyStep | `F0 3F F7` |
| Frame de firmware | `F0 <22 chars ASCII hex> F7` |
| ACK por frame | `F0 1A F7` |
| ACK final | `F0 77 F7` |

Wireshark fue lo que destrabó el proyecto entero: me dejó ver que era MIDI/SysEx, encontrar el handshake, entender el tamaño y la forma de los frames, ver los ACKs y, en definitiva, **reproducir el comportamiento del updater con mi propio código**. Sin esa captura habría sabido generar binarios, pero sin una forma confiable de meterlos al equipo.

---

## 5. El formato `.led` y el cargador KS-BootBridge

El firmware oficial usa extensión `.led`. No es un binario plano: lleva los datos en **ASCII hex**. Lo que observé:

```text
Frame raw:        2078 bytes
Frame ASCII hex:  4156 caracteres
Cola final:       12 bytes raw = 24 caracteres hex
```

Con eso pude armar un empaquetador que toma un binario propio y lo mete en una estructura compatible con el bootloader, y un cargador al que llamé **KS-BootBridge** porque hace de puente entre el firmware custom y el bootloader MIDI/SysEx del KeyStep.

Lo que hace el cargador, paso a paso:

1. Abre el puerto raw MIDI.
2. Envía el handshake.
3. Espera la respuesta del KeyStep.
4. Lee/divide los frames del `.led`.
5. Manda cada frame como SysEx.
6. Espera el ACK de cada frame.
7. Espera el ACK final.

Uso conceptual:

```bash
python3 ks_bootbridge.py firmware.led --port hw:2,0,0
```

---

## 6. Lo que encontré en Ghidra

Cargué el firmware en Ghidra como **ARM Cortex-M Thumb**, con un detalle importante: la aplicación no empieza en `0x08000000`, sino después de la zona del bootloader. La base correcta es:

```text
0x08004000
```

Con la base bien puesta, la tabla de vectores tenía sentido (stack pointer en RAM, reset vector apuntando a `0x08004xxx`) y pude reconstruir la ruta de arranque:

```text
Reset trampoline → early init → runtime init → App_Main → init de periféricos → loop principal
```

Fui renombrando funciones para entender los subsistemas. Algunas de las más útiles:

```text
App_Main
MX_SPI2_Init
MX_DAC_or_ADC_Init
MX_UART_MIDI_Init
MX_USB_Device_Init_like
InputScanner8_Init
InputScanner_SelectIndex
InputScanner_ReadValue
InputScanner_ResetSelectLines
KeyboardScanner_Update
MIDI_NoteOn_Build / NoteOff / ControlChange / PitchBend
```

Eso me dejó separar el firmware en bloques: init de hardware, escaneo de teclado, MIDI, control values, sequencer, DAC/CV y USB. Pero lo que de verdad importaba estaba escondido en una sola línea.

---

## 7. El gran descubrimiento: la matriz del teclado

La función que lo cambió todo fue esta:

```c
InputScanner8_Init(DAT_080196e4, DAT_080196e0, 8, DAT_080196dc, 8);
```

Reconstruyendo la firma:

```c
InputScanner8_Init(scanner, selectPort, selectShift, readPort, readShift);
```

Resolviendo las direcciones:

```text
DAT_080196e4 = 0x20000F44   → scanner
DAT_080196e0 = 0x40011800   → GPIOE  (líneas de selección)
DAT_080196dc = 0x40011400   → GPIOD  (líneas de lectura)
```

Y ahí estaba el cableado real del teclado:

```text
PE8–PE15 = selección
PD8–PD15 = lectura
```

### Cómo funciona eléctricamente

El scanner es **activo en bajo**: pones todas las líneas de selección en alto, bajas una sola, lees las de entrada, inviertes y guardas. Repites para las 8 filas.

```c
for (row = 0; row < 8; row++) {
    GPIOE->ODR |= 0xFF00;              // todo inactivo
    GPIOE->ODR &= ~(1 << (8 + row));   // seleccionar fila
    delay_small();
    rows[row] = ~(GPIOD->IDR >> 8) & 0xFF;
}
```

### De la matriz a las teclas

El KeyStep tiene 32 teclas y **cada una tiene dos contactos** (A y B). La cuenta cuadra perfecto:

```text
32 teclas × 2 contactos = 64
8 selects × 8 reads      = 64
```

Las líneas de selección se reparten en pares (un contacto A y uno B por grupo de 8 teclas):

```text
PE8/PE9   → grupo 0 (A/B)
PE10/PE11 → grupo 1 (A/B)
PE12/PE13 → grupo 2 (A/B)
PE14/PE15 → grupo 3 (A/B)
```

Y las de lectura (`PD8–PD15`) seleccionan la columna. La fórmula para cualquier tecla:

```text
grupo   = keyIndex / 8
columna = keyIndex % 8

Contacto A:  PE = 8 + grupo*2       ,  PD = 8 + columna
Contacto B:  PE = 8 + grupo*2 + 1   ,  PD = 8 + columna
```

Por ejemplo, la tecla 0 es `PE8+PD8` (A) y `PE9+PD8` (B); la tecla 31 es `PE14+PD15` (A) y `PE15+PD15` (B).

> **Lo que aprendí por las malas:** antes de esto intenté mapear pines GPIO sueltos "a ciegas", y solo conseguí falsos positivos. El teclado **no** se puede leer mirando pines individuales; hay que activar `PE8–PE15` una por una y leer `PD8–PD15`. Ese fue el salto de las pruebas erráticas a una lectura real.

---

## 8. Primer audio y primeras teclas

Con el cargador y la matriz resueltos, fui validando por partes:

- **V4 — escribir en el DAC:** hice que la salida alternara voltaje. Funcionó → el firmware propio podía controlar el DAC.
- **V5 — audio básico:** generé una señal audible simple. La unidad sonó → la cadena bootloader → uploader → firmware estaba completa.
- **V22 — escaneo real:** usé `PE8–PE15` / `PD8–PD15` y al apretar teclas sonaban beeps → la matriz descubierta era correcta.
- **V23 — una nota por tecla:** asigné una frecuencia a cada tecla y cada una dio su nota. **Este fue el momento en que el KeyStep dejó de ser controlador y pasó a ser instrumento.**

---

## 9. Probando motores de síntesis

Acá vino mucha prueba y error. Resumen honesto de lo que probé:

- **Onda cuadrada:** baratísima en CPU, pero áspera.
- **Wavetable / seno:** más suave, pero débil y poco expresiva.
- **Moog-ish** (doble sierra + suboscilador + filtro digital simple + envolvente + saturación): útil como lead/bajo, aunque limitado por el DAC y la salida CV.
- **Polifonía (4 voces):** técnicamente funcionó, pero sonó como una masa densa. Varias voces + sub + saturación sobre una sola salida DAC simple = mezcla turbia.
- **FM / Yamaha-ish:** modulación de frecuencia simple. Más digital y brillante.
- **Rhodes-ish:** no convenció. Un Rhodes pide dinámica, resonancia, preamp, amplificación y efectos; por DAC/CV no sale.
- **MicroFreak-ish** (wavetable morph + FM suave + wavefolding + movimiento lento + vibrato): el carácter que mejor le calza a este hardware.

La conclusión general: en vez de pelear contra el hardware imitando timbres analógicos complejos, conviene abrazar lo digital, que es donde el STM32 y el DAC se sienten cómodos.

---

## 10. MultiEngine y controles ocultos

En la **V35** junté varios motores en un solo firmware, para dejar de compilar una versión por timbre:

```text
1 = MicroFreak-ish digital
2 = Yamaha FM
3 = Moog lead
4 = Acid bass
5 = Digital bell/pad
```

Como todavía no mapeé los botones del panel, los controles van por combinaciones con **la tecla más grave**:

| Combinación | Acción |
|---|---|
| Grave + 1…5 | elegir motor (MicroFreak / FM / Moog / Acid / Bell-Pad) |
| Grave + 6 / 7 / 8 | octava abajo / normal / arriba |
| Grave + 9 / 10 / 11 | timbre oscuro / normal / brillante |
| Grave + 12 / 13 / 14 | vibrato off / normal / profundo |
| Grave + 15 | hold / latch |
| Grave + 16 / 17 | velocity/accent off / on |

---

## 11. Mod CV como salida principal

Durante varias versiones di por hecho que el audio iba por `Pitch CV / DAC1 / PA4`. La prueba real me corrigió: en mi unidad esa salida **no** servía como audio útil, y la buena era `Mod CV / DAC2 / PA5`.

Desde la **V37** en adelante el proyecto asume:

```text
Audio principal = Mod CV / DAC2 / PA5
Pitch CV        = ignorado / reservado para pruebas futuras
```

Lo dejo bien marcado para que nadie repita mi confusión.

---

## 12. Velocity por doble contacto

Que cada tecla tenga **dos contactos** abre la puerta a medir con qué fuerza tocas. La idea:

```text
El contacto A se activa primero, el B después.
La diferencia de tiempo aproxima la fuerza:
  diferencia corta = golpe fuerte
  diferencia larga = golpe suave
```

Eso se puede mapear musicalmente a más volumen, más brillo, más cantidad de FM o un ataque más marcado. La **V40** implementó una primera aproximación tipo acento. Todavía es tosco, pero el principio funciona.

---

## 13. Tabla de versiones

| Versión | Qué probé | Resultado |
|---|---|---|
| V4  | DAC alternando voltaje | validó control del DAC |
| V5  | Audio básico | validó generación audible |
| V22 | Scanner real PE/PD con beeps | validó la matriz |
| V23 | Una nota por tecla | primer instrumento funcional |
| V24 | Wavetable suave | menos áspero |
| V25 | Moog-ish | lead/bajo más útil |
| V26 | 4 voces | polifonía densa |
| V27 | Dúo limpio | más claro |
| V29 | Mono lead + portamento + arp | más usable |
| V30 | FM + arp | digital brillante |
| V31 | FM sin arp | más directo |
| V32 | Rhodes-ish | no convincente |
| V33 | Rhodes MK2-ish | mejor, pero limitado |
| V34 | MicroFreak-ish | el enfoque más adecuado |
| V35 | MultiEngine | 5 motores funcionando |
| V36 | MultiEngine + octava | más usable |
| V37 | Performance + Mod CV | confirmó la salida Mod CV |
| V38 | Mod CV audio + Pitch auxiliar | Pitch no útil |
| V39 | Solo Mod CV | arquitectura corregida |
| V40 | Velocity / acento | primera dinámica |

---

## 14. Arquitectura final del firmware

```text
Reset
 → init de reloj
 → init GPIO
 → init DAC
 → configuración de la matriz PE/PD
 → loop principal:
      escaneo de teclado
      lectura de contactos A/B
      detección de combinaciones
      selección de motor
      octava / timbre / vibrato / hold
      render de síntesis
      escritura a DAC2 / PA5 / Mod CV
```

En código, el loop es tan simple como:

```c
while (1) {
    KeyboardMatrix_ScanRows(rows);
    KeyState_Update(rows);
    Controls_Update();
    sample = Synth_Render();
    DAC2_Write(sample);
}
```

---

## 15. Comparación honesta con sintes comerciales

Esto **no** es un MicroFreak, un MiniFreak ni un Reface DX. La descripción justa sería:

> **KeyFreak = un mono-synth digital DIY sobre STM32, estilo "MicroFreak-lite" experimental.**

**Lo que sí tiene:** teclado físico de 32 teclas leído por firmware propio, varios motores digitales simples, salida Mod CV usada como audio, control de octava/timbre/vibrato/hold y una primera aproximación a velocity.

**Lo que no tiene:** códec de audio dedicado, salida de línea real, filtro analógico, efectos integrados, DSP potente, un motor FM completo ni polifonía de calidad comercial.

Una mejora futura interesante sería un **DAC externo PCM5102** vía I2S (BCLK/LRCK/DATA), que daría menos ruido, mejor rango dinámico y un line-out limpio. El reto está en encontrar pines I2S libres, modificar hardware, escribir el driver y validar un sample rate estable. Ojo: el PCM5102 **no** se conecta a Mod CV; es una ruta de audio aparte.

---

## 16. Trabajo futuro

Lo que me gustaría seguir explorando (y donde se agradecen aportes):

1. Mapear los botones reales del panel.
2. Mapear las touch strips.
3. Mejorar la velocity real por doble contacto.
4. Presets más musicales y refinar el MultiEngine.
5. Un modo parafónico limpio.
6. Investigar I2S con PCM5102.
7. Instalador más seguro + modo de recuperación.
8. Documentar el hardware con fotos y diagramas.
9. Separar el firmware en módulos y agregar tests de teclado/DAC.
10. Control MIDI externo y demos de audio.

---

## 17. Estructura del repo

```text
keystep-custom-synth/
├── README.md
├── LICENSE
├── docs/
│   ├── bootloader_protocol.md
│   ├── firmware_format_led.md
│   ├── ghidra_analysis.md
│   ├── keyboard_matrix.md
│   ├── modcv_audio_output.md
│   ├── firmware_versions.md
│   └── safety_and_legal.md
├── firmware/
│   ├── v04_dac_test/
│   ├── v05_audio_test/
│   ├── v22_keyboard_scan/
│   ├── v23_single_note/
│   ├── v35_multiengine/
│   ├── v39_modcv_performance/
│   └── v40_velocity/
├── tools/
│   ├── ks_bootbridge/
│   └── led_packer/
├── hardware/
│   ├── board_notes.md
│   ├── pinout.md
│   └── pcm5102_notes.md
└── images/
```

**Qué incluir:** código C propio, scripts Python propios, documentación, fotos propias de la placa, diagramas propios, notas resumidas de Ghidra, el pinout descubierto, instrucciones de compilación y advertencias.

**Qué NO incluir:** firmware oficial `.led`, dumps propietarios, binarios extraídos de Arturia, capturas con información redistribuible sensible, ni marcas usadas como si fueran propias.

---

## 18. Cierre

KeyFreak demuestra que un controlador "cerrado" como el KeyStep Mk1 puede transformarse en una pequeña plataforma abierta de síntesis. Pero para mí lo más valioso no fue que sonara, sino haber reconstruido el camino completo:

```text
identificar el hardware
 → entender el bootloader
 → capturar el protocolo
 → escribir un uploader propio
 → analizar el firmware en Ghidra
 → descubrir la matriz del teclado
 → cargar firmware propio
 → generar audio por Mod CV
 → construir motores de síntesis
```

No reemplaza a ningún sinte comercial moderno, y no pretende hacerlo. El punto es la metodología —inspeccionar, capturar, analizar, equivocarse, documentar y construir de a poco— y dejarla abierta para que otra persona la tome desde donde yo la dejé.

Si lo replicas, lo mejoras o te quedas pegado en algún paso, los issues y forks son bienvenidos. Para eso lo publico. 🙂

---

*Hecho por Eduardo Mansilla como proyecto de aprendizaje sobre firmware embebido, ingeniería inversa y síntesis digital. Sin afiliación con Arturia; las marcas se mencionan solo de forma descriptiva.*
