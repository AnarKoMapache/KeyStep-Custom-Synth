# KeyFreak / KeyStep Custom Synth  
## Paper técnico completo: conversión experimental de un Arturia KeyStep Mk1 en sintetizador digital autónomo mediante ingeniería inversa, análisis de protocolo y firmware alternativo

**Autor del proyecto:** Mapache Bastardx
**Nombre propuesto del proyecto:** KeyFreak  
**Nombre del cargador custom:** KS-BootBridge  
**Dispositivo intervenido:** Arturia KeyStep Mk1 / KeyStep 32  
**Microcontrolador identificado:** STM32F103VCT6  
**Arquitectura:** ARM Cortex-M3  
**Firmware custom:** C bare-metal / STM32 HAL parcial  
**Herramientas principales:** Ghidra, Wireshark/USBPcap, Python, raw MIDI/SysEx, GCC ARM Embedded  
**Salida de audio funcional validada:** Mod CV / DAC2 / PA5  
**Estado:** proof of concept funcional y cargable por bootloader  
**Fecha de documentación:** 2026-06-21  

---

# 1. Resumen ejecutivo

Este documento describe de manera completa el proceso mediante el cual se transformó experimentalmente un **Arturia KeyStep Mk1** en una plataforma de síntesis digital autónoma. El KeyStep fue diseñado originalmente como controlador MIDI/CV, pero internamente posee un microcontrolador STM32F103VCT6 capaz de ejecutar código propio, leer matrices de teclado, controlar GPIO, usar DACs internos y generar señales de audio simples.

El proyecto logró cuatro hitos principales:

1. **Replicar el proceso de carga de firmware** usado por el updater oficial mediante captura con Wireshark y análisis de mensajes USB-MIDI/SysEx.
2. **Construir un cargador custom de firmware**, denominado `KS-BootBridge`, capaz de enviar imágenes `.led` experimentales al KeyStep en modo bootloader.
3. **Descubrir la matriz física real del teclado** mediante ingeniería inversa en Ghidra, identificando las líneas de selección y lectura:
   - `PE8–PE15` como líneas de selección.
   - `PD8–PD15` como líneas de lectura.
4. **Desarrollar firmware alternativo funcional**, capaz de leer las 32 teclas, generar audio por Mod CV y ejecutar motores de síntesis digitales simples.

El resultado práctico fue un KeyStep capaz de funcionar como un mini sintetizador digital experimental, con motores tipo MicroFreak-ish, Yamaha FM-ish, Moog-ish, acid bass, bell/pad, octavas, vibrato, brillo, hold/latch y una primera aproximación a velocity por doble contacto.

---

# 2. Objetivo del proyecto

El objetivo inicial fue responder una pregunta simple:

> ¿Puede un Arturia KeyStep Mk1 ejecutar firmware propio y generar sonido por sí mismo, dejando de ser solo un controlador?

A partir de esa pregunta se definieron objetivos técnicos:

- Identificar el microcontrolador principal.
- Entender el bootloader y el formato de actualización.
- Cargar código propio sin usar herramientas oficiales modificadas.
- Leer el teclado físico.
- Producir audio desde el DAC interno.
- Construir motores de síntesis simples.
- Documentar el proceso para GitHub sin distribuir material propietario.

---

# 3. Alcance y límites

Este proyecto **no** busca:

- Reemplazar comercialmente el firmware oficial.
- Distribuir firmware original de Arturia.
- Clonar productos Arturia.
- Distribuir dumps de memoria o binarios propietarios.
- Garantizar seguridad de uso en todos los KeyStep.

Este proyecto **sí** busca:

- Documentar una investigación técnica.
- Mostrar cómo se identificó la matriz del teclado.
- Explicar cómo se replicó el protocolo de carga.
- Crear firmware propio cargable.
- Usar el KeyStep como plataforma educativa de firmware embebido y síntesis digital.

---

# 4. Advertencia legal y de seguridad

Modificar el firmware de un equipo comercial puede dejarlo inutilizable. Todo el proceso debe considerarse experimental.

Buenas prácticas eléctricas:

```text
No conectar audífonos directamente a salidas CV.
Usar mixer, interfaz de audio o entrada de sintetizador con volumen bajo.
No soldar modificaciones permanentes sin verificar pines y voltajes.
No alimentar módulos externos desde puntos internos sin medir.
```

---

# 5. Hardware identificado

## 5.1 Microcontrolador

A partir de inspección física de la placa se identificó el microcontrolador:

```text
STM32F103VCT6
ARM Cortex-M3
Frecuencia típica: hasta 72 MHz
Flash: 256 KB aprox.
RAM: 48 KB aprox.
DAC: 2 canales de 12 bits
```

## 5.2 DACs relevantes

El STM32F103VCT6 posee dos salidas DAC:

```text
PA4 = DAC_OUT1 = DAC1
PA5 = DAC_OUT2 = DAC2
```

Durante el proyecto se asumió inicialmente que `Pitch CV / DAC1 / PA4` sería la salida de audio principal. Sin embargo, las pruebas prácticas corrigieron esta hipótesis:

```text
Salida de audio funcional validada:
Mod CV / DAC2 / PA5

Pitch CV / DAC1 / PA4:
No resultó útil como salida de audio en la unidad probada.
```

Por ello, las versiones recientes se enfocaron en:

```text
Audio principal = Mod CV / DAC2 / PA5
Pitch CV = ignorado o reservado para futuras pruebas
```

---

# 6. Modo bootloader del KeyStep

Se descubrió que el KeyStep entra en modo bootloader manteniendo una combinación de botones mientras se conecta por USB:

```text
Record + Stop + Play/Pause + conectar USB
```

En Linux, el dispositivo fue reconocido como updater:

```text
USB:
Arturia KeyStep Updater

MIDI:
KeyStep Updater MIDI 1
```

Este modo permite recibir firmware mediante mensajes MIDI/SysEx.

---

# 7. Captura del protocolo con Wireshark

## 7.1 Objetivo de la captura

Para poder cargar firmware custom, era necesario entender cómo el software oficial enviaba el firmware al KeyStep. Se usó Wireshark/USBPcap para capturar el tráfico durante un proceso de actualización oficial.

## 7.2 Flujo observado

El flujo general del updater fue:

```text
PC / Arturia updater
↓
Handshake SysEx
↓
Respuesta del KeyStep
↓
Frames de firmware
↓
ACK por cada frame
↓
ACK final
```

## 7.3 Mensajes relevantes

Se identificaron los siguientes mensajes:

```text
Handshake enviado por el PC:
F0 5A 57 6E 28 3C 4E 3F F7

Respuesta del KeyStep:
F0 3F F7

Frame de firmware:
F0 <22 caracteres ASCII hex> F7

ACK por frame:
F0 1A F7

ACK final:
F0 77 F7
```

## 7.4 Importancia de Wireshark

Wireshark fue crítico porque permitió:

- Ver que la comunicación era MIDI/SysEx.
- Identificar el handshake.
- Identificar el tamaño y estructura de frames.
- Identificar los ACKs.
- Reproducir el comportamiento del updater oficial.
- Crear un cargador propio.

Sin esta captura, el proyecto habría quedado limitado a generar binarios, pero sin una forma confiable de cargarlos al KeyStep.

---

# 8. Formato del archivo `.led`

El archivo oficial de firmware usa extensión `.led`. Durante el análisis se observó:

```text
El .led no es un binario plano.
Contiene datos en ASCII hex.
```

Características observadas:

```text
Frame raw: 2078 bytes
Frame ASCII hex: 4156 caracteres
Cola final: 12 bytes raw = 24 caracteres hex
```

Esto permitió construir un empaquetador que toma un binario custom y lo inserta en una estructura compatible con el bootloader.

---

# 9. KS-BootBridge: cargador custom de firmware

## 9.1 Nombre

Se propuso el nombre:

```text
KS-BootBridge
```

Porque funciona como puente entre:

```text
Firmware custom .led
↓
Bootloader MIDI/SysEx del KeyStep
```

## 9.2 Función

El cargador hace:

1. Abre el puerto raw MIDI.
2. Envía el handshake.
3. Espera respuesta del KeyStep.
4. Divide o lee frames `.led`.
5. Envía cada frame como SysEx.
6. Espera ACK por frame.
7. Espera ACK final.

## 9.3 Uso conceptual

```bash
python3 ks_bootbridge.py firmware.led --port hw:2,0,0
```

Durante el desarrollo se usó una herramienta equivalente con nombre más largo:

```bash
python3 keystep_rawmidi_handshake_uploader.py firmware.led --port hw:2,0,0
```

---

# 10. Carga del firmware en Ghidra

## 10.1 Base correcta

El firmware fue analizado en Ghidra como ARM Cortex-M Thumb. Se corrigió la base de aplicación a:

```text
0x08004000
```

Esta dirección fue importante porque la aplicación no comienza en `0x08000000`, sino después de una zona reservada para bootloader.

## 10.2 Vector table

Se observó que el binario inicia con valores típicos de tabla de vectores:

```text
Stack pointer inicial en RAM
Reset vector apuntando a zona 0x08004xxx
```

## 10.3 Ruta de inicio observada

Se reconstruyó una ruta aproximada:

```text
Reset trampoline
↓
early init
↓
runtime init
↓
App_Main
↓
inicialización de periféricos
↓
scheduler / loop principal
```

---

# 11. Funciones relevantes identificadas en Ghidra

Durante el análisis se renombraron múltiples funciones para entender mejor el firmware.

Algunas funciones relevantes:

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
MIDI_NoteOn_Build
MIDI_NoteOff_Build
MIDI_ControlChange_Build
MIDI_PitchBend_Build
```

Esto permitió separar el firmware en subsistemas:

```text
Inicialización de hardware
Escaneo de teclado
MIDI
Control values
Sequencer
DAC/CV
USB
```

---

# 12. Descubrimiento de la matriz del teclado

## 12.1 Función clave

En Ghidra se encontró una llamada crucial:

```c
InputScanner8_Init(DAT_080196e4, DAT_080196e0, 8, DAT_080196dc, 8);
```

La firma reconstruida fue:

```c
InputScanner8_Init(scanner, selectPort, selectShift, readPort, readShift);
```

Por tanto:

```text
scanner     = DAT_080196e4
selectPort  = DAT_080196e0
selectShift = 8
readPort    = DAT_080196dc
readShift   = 8
```

## 12.2 Resolución de direcciones

Se resolvieron los valores:

```text
DAT_080196e4 = 0x20000F44
DAT_080196e0 = 0x40011800
DAT_080196dc = 0x40011400
```

Mapeo STM32:

```text
0x40011800 = GPIOE
0x40011400 = GPIOD
```

Conclusión:

```text
PE8–PE15 = selección
PD8–PD15 = lectura
```

---

# 13. Funcionamiento eléctrico de la matriz

El scanner funciona activo en bajo:

```text
1. Poner PE8–PE15 en alto.
2. Bajar una línea PE específica.
3. Leer PD8–PD15.
4. Invertir lectura.
5. Guardar resultado.
6. Repetir para las 8 líneas.
```

Pseudocódigo:

```c
for (row = 0; row < 8; row++) {
    GPIOE->ODR |= 0xFF00;              // todo inactivo
    GPIOE->ODR &= ~(1 << (8 + row));   // seleccionar fila
    delay_small();
    rows[row] = ~(GPIOD->IDR >> 8) & 0xFF;
}
```

---

# 14. Relación entre matriz y teclas

El KeyStep posee 32 teclas, cada una con dos contactos.

```text
32 teclas x 2 contactos = 64 contactos
8 selects x 8 reads = 64 lecturas
```

Distribución:

```text
PE8  = contacto A grupo 0
PE9  = contacto B grupo 0
PE10 = contacto A grupo 1
PE11 = contacto B grupo 1
PE12 = contacto A grupo 2
PE13 = contacto B grupo 2
PE14 = contacto A grupo 3
PE15 = contacto B grupo 3
```

Cada grupo cubre 8 teclas mediante:

```text
PD8–PD15
```

Fórmula:

```text
keyIndex = 0..31
grupo = keyIndex / 8
columna = keyIndex % 8

Contacto A:
PE = 8 + grupo * 2
PD = 8 + columna

Contacto B:
PE = 8 + grupo * 2 + 1
PD = 8 + columna
```

Ejemplo:

```text
Tecla 0:
A = PE8 + PD8
B = PE9 + PD8

Tecla 7:
A = PE8 + PD15
B = PE9 + PD15

Tecla 8:
A = PE10 + PD8
B = PE11 + PD8

Tecla 31:
A = PE14 + PD15
B = PE15 + PD15
```

---

# 15. Primeros intentos fallidos

Antes de descubrir la matriz real, se intentaron mappers GPIO a ciegas. Estos fallaron o generaron falsos positivos porque el teclado no podía leerse simplemente observando pines individuales.

Problema:

```text
Leer GPIO suelto no basta.
Hay que activar líneas PE8–PE15 una por una y leer PD8–PD15.
```

Este aprendizaje fue clave para pasar de pruebas erráticas a lectura real del teclado.

---

# 16. Primeras pruebas de DAC

## 16.1 V4: alternancia de voltaje

Objetivo:

```text
Confirmar que el firmware custom podía escribir en DAC.
```

Resultado:

```text
La salida alternó voltaje correctamente.
```

## 16.2 V5: audio básico

Objetivo:

```text
Generar señal audible simple.
```

Resultado:

```text
La unidad produjo audio.
```

Estas pruebas confirmaron que:

```text
El firmware custom ejecuta.
El DAC puede ser controlado.
La cadena bootloader → uploader → firmware funciona.
```

---

# 17. Validación de teclado

## 17.1 V22: beeps por matriz real

Objetivo:

```text
Usar PE8–PE15 y PD8–PD15 para detectar teclas.
```

Resultado:

```text
Al presionar teclas, sonaron beeps.
```

Conclusión:

```text
La matriz descubierta era correcta.
```

## 17.2 V23: nota única por tecla

Objetivo:

```text
Asignar una frecuencia a cada tecla.
```

Resultado:

```text
Cada tecla generó una nota continua.
```

Este fue el primer punto donde el KeyStep funcionó como instrumento.

---

# 18. Desarrollo de motores de síntesis

Se probaron distintos enfoques de síntesis.

## 18.1 Onda cuadrada

Ventaja:

```text
Muy barata en CPU.
```

Desventaja:

```text
Sonido áspero.
```

## 18.2 Wavetable/seno

Ventaja:

```text
Más suave.
```

Desventaja:

```text
Sonido débil o poco expresivo.
```

## 18.3 Moog-ish

Idea:

```text
Doble sierra
suboscilador
filtro digital simple
envolvente
saturación suave
```

Resultado:

```text
Más útil como lead/bajo, pero limitado por DAC y salida CV.
```

## 18.4 Polifonía

Se implementaron varias voces mezcladas.

Resultado:

```text
Técnicamente funcionó, pero sonó como masa densa.
```

Causa:

```text
Varias voces + sub + saturación + una salida DAC simple = mezcla poco clara.
```

## 18.5 FM / Yamaha-ish

Idea:

```text
Modulación de frecuencia simple.
```

Resultado:

```text
Más digital y brillante.
```

## 18.6 Rhodes-ish

Se intentó imitar electric piano.

Resultado:

```text
No fue convincente.
```

Razón:

```text
Un Rhodes requiere dinámica, resonancia, preamp, amplificador, efectos y respuesta compleja.
El KeyStep por DAC/CV no lo reproduce bien.
```

## 18.7 MicroFreak-ish

Se optó por un carácter digital más propio:

```text
wavetable morph
FM suave
wavefolding
movimiento lento
vibrato
```

Resultado:

```text
Más adecuado para el hardware.
```

---

# 19. MultiEngine

La versión V35 consolidó varios motores en un solo firmware:

```text
1 = MicroFreak-ish digital
2 = Yamaha FM
3 = Moog lead
4 = Acid bass
5 = Digital bell/pad
```

Esto fue importante porque permitió dejar de compilar una versión distinta para cada timbre.

---

# 20. Controles ocultos desde el teclado

Como aún no se mapearon todos los botones del panel, se implementaron combinaciones con la tecla más grave:

```text
Tecla más grave + 1 = MicroFreak-ish
Tecla más grave + 2 = Yamaha FM
Tecla más grave + 3 = Moog lead
Tecla más grave + 4 = Acid bass
Tecla más grave + 5 = Digital bell/pad

Tecla más grave + 6 = octava abajo
Tecla más grave + 7 = octava normal
Tecla más grave + 8 = octava arriba

Tecla más grave + 9  = timbre oscuro
Tecla más grave + 10 = timbre normal
Tecla más grave + 11 = timbre brillante

Tecla más grave + 12 = vibrato off
Tecla más grave + 13 = vibrato normal
Tecla más grave + 14 = vibrato profundo

Tecla más grave + 15 = hold/latch
Tecla más grave + 16 = velocity/accent off
Tecla más grave + 17 = velocity/accent on
```

---

# 21. Salida Mod CV como audio principal

Durante varias iteraciones se creyó que `Pitch CV / DAC1 / PA4` era la salida a usar. La prueba real corrigió esto.

Estado final validado:

```text
Mod CV funciona como salida de audio.
Pitch CV no funcionó como audio útil.
```

Por tanto, desde las últimas versiones:

```text
Audio principal = Mod CV / DAC2 / PA5
```

Este punto debe quedar claramente documentado en el repositorio para evitar confusión.

---

# 22. Velocity por doble contacto

El teclado tiene dos contactos por tecla. Esto permite intentar medición de velocidad.

Idea:

```text
Contacto A se activa primero.
Contacto B se activa después.
La diferencia de tiempo aproxima la fuerza de pulsación.
```

Interpretación:

```text
Diferencia corta = golpe fuerte
Diferencia larga = golpe suave
```

Uso musical:

```text
Más fuerza = más volumen
Más fuerza = más brillo
Más fuerza = más FM amount
Más fuerza = ataque más fuerte
```

La versión V40 implementó una primera aproximación tipo acento.

---

# 23. Versiones desarrolladas

| Versión | Descripción | Resultado |
|---|---|---|
| V4 | DAC alternando voltaje | Validó control DAC |
| V5 | Audio básico | Validó generación audible |
| V22 | Scanner real PE/PD con beeps | Validó matriz |
| V23 | Nota única por tecla | Primer instrumento funcional |
| V24 | Wavetable suave | Menos áspero |
| V25 | Moog-ish | Lead/bajo más útil |
| V26 | 4 voces | Polifonía densa |
| V27 | Duo limpio | Más claro |
| V29 | Mono lead + portamento + arp | Más usable |
| V30 | FM + arp | Digital brillante |
| V31 | FM sin arp | Más directo |
| V32 | Rhodes-ish | No convincente |
| V33 | Rhodes MK2-ish | Mejor pero limitado |
| V34 | MicroFreak-ish | Enfoque más adecuado |
| V35 | MultiEngine | 5 motores funcionando |
| V36 | MultiEngine + octava | Más usable |
| V37 | Performance + Mod CV | Confirmó salida Mod CV |
| V38 | Mod CV audio + Pitch auxiliar | Pitch no útil |
| V39 | Mod CV only performance | Arquitectura corregida |
| V40 | Velocity/acento | Primera dinámica |

---

# 24. Arquitectura final del firmware experimental

```text
Reset
↓
Inicialización de reloj
↓
Inicialización GPIO
↓
Inicialización DAC
↓
Configuración matriz PE/PD
↓
Loop principal
  ↓
  Escaneo de teclado
  ↓
  Lectura de contactos A/B
  ↓
  Detección de combinaciones
  ↓
  Selección de motor
  ↓
  Aplicación de octava/timbre/vibrato/hold
  ↓
  Render de síntesis
  ↓
  Escritura a DAC2 / PA5 / Mod CV
```

Pseudocódigo:

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

# 25. Comparación con sintetizadores comerciales

El proyecto no equivale a un MicroFreak, MiniFreak o Reface DX completo. La comparación más honesta:

```text
KeyFreak = MicroFreak-lite experimental / DIY STM32 mono-synth
```

Tiene:

```text
- teclado físico de 32 teclas
- matriz real leída por firmware propio
- varios motores digitales simples
- salida Mod CV usada como audio
- control de octava, timbre, vibrato y hold
- primera aproximación a velocity
```

No tiene:

```text
- codec de audio dedicado
- salida de audio real de línea
- filtro analógico
- efectos integrados
- DSP potente
- motor FM completo
- polifonía de calidad comercial
```

---

# 26. Posible mejora con PCM5102

Un DAC externo PCM5102 podría mejorar la calidad de audio, pero no se conecta a Mod CV.

El PCM5102 requiere:

```text
I2S digital:
BCLK
LRCK
DATA
GND
3.3V
```

Ruta posible:

```text
STM32F103
↓
I2S
↓
PCM5102
↓
salida de audio line out
```

Ventajas esperadas:

```text
menos ruido
mejor rango dinámico
mejor nivel de salida
audio más limpio
```

Desafíos:

```text
identificar pines I2S disponibles
modificar hardware
escribir driver I2S
validar sample rate estable
```

---

# 27. Estructura recomendada para GitHub

```text
keystep-custom-synth/
├── README.md
├── PAPER.md
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

---

# 28. Nombres recomendados

Proyecto:

```text
KeyFreak
```

Repositorio:

```text
keystep-custom-synth
```

Cargador:

```text
KS-BootBridge
```

Script:

```text
ks_bootbridge.py
```

Descripción corta:

```text
Experimental STM32F103 firmware and reverse-engineering toolkit that turns the Arturia KeyStep Mk1 into a small digital synthesizer using its keyboard matrix and Mod CV output.
```

---

# 29. Qué debería incluirse y qué no

## Incluir

```text
Código C propio
Scripts Python propios
Documentación
Fotos propias de la placa
Diagramas propios
Notas de Ghidra resumidas
Pinout descubierto
Instrucciones de compilación
Advertencias
```

## No incluir

```text
Firmware oficial .led
Dumps propietarios completos
Binarios extraídos de Arturia
Capturas con información redistribuible sensible
Marcas como si fueran propias
```

---

# 30. Trabajo futuro

Siguientes pasos técnicos:

1. Mapear botones reales del panel.
2. Mapear touch strips.
3. Mejorar velocity real por doble contacto.
4. Crear presets más musicales.
5. Refinar MultiEngine.
6. Crear modo paraphonic limpio.
7. Investigar I2S con PCM5102.
8. Crear instalador más seguro.
9. Agregar modo de recuperación.
10. Documentar hardware con fotos y diagramas.
11. Separar firmware en módulos.
12. Crear tests de teclado y DAC.
13. Agregar control MIDI externo si corresponde.
14. Crear demos de audio.

---

# 31. Conclusión

El proyecto KeyFreak demuestra que el Arturia KeyStep Mk1 puede transformarse experimentalmente en una plataforma de síntesis digital mediante firmware alternativo. El logro principal no fue solo generar sonido, sino reconstruir el camino completo:

```text
hardware identificado
↓
bootloader entendido
↓
protocolo capturado
↓
uploader custom creado
↓
firmware analizado en Ghidra
↓
matriz de teclado descubierta
↓
firmware propio cargado
↓
audio generado por Mod CV
↓
motores de síntesis implementados
```

El dispositivo resultante no reemplaza a un sintetizador comercial moderno, pero sí demuestra que un controlador cerrado puede convertirse en una plataforma abierta de aprendizaje y experimentación.

El valor del proyecto está en la metodología: inspección, captura, análisis, prueba, error, documentación y construcción progresiva de un sistema funcional.
