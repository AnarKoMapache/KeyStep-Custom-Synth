# Firmware versions

## Línea de desarrollo principal

| Versión | Objetivo | Estado |
|---|---|---|
| V4 | Prueba directa de DAC | Alternancia de voltaje validada |
| V5 | Audio busy loop | Referencia inicial de audio |
| V22 | Scanner real PE/PD con beeps | Confirmó lectura del teclado |
| V23 | Una nota por tecla | Monofónico básico funcional |
| V24 | Seno/wavetable suave | Funcionó, pero poco carácter |
| V25 | Moog-ish mono | Más sintético/gordo |
| V26/V26B | Polifonía | Funcionó, pero se volvió una masa sonora |
| V27 | Duo poly más limpio | Menos masa |
| V29 | Mono lead + portamento + arp | Mejor como lead, pero se retiró el arp después |
| V30/V31 | Yamaha/DX-ish | FM digital experimental |
| V32/V33 | Rhodes-ish | No logró acercarse lo suficiente a Rhodes |
| V34/V34B | MicroFreak-ish | Digital/wavetable/FM/wavefolding |
| V35 | MultiEngine | 5 motores cambiables funcionando |
| V36 | MultiEngine + octava | Agregó octava |
| V37 | Performance + Mod CV | Confirmó salida útil por Mod CV |
| V38 | Audio Mod CV + Pitch CV auxiliar | Pitch CV no fue útil en prueba |
| V39 | Mod CV only performance | Enfocado a salida Mod CV |
| V40 | Velocity/acento por doble contacto | Versión experimental más completa |

## Estado recomendado actual

```text
V40 = versión base recomendada para seguir desarrollando
```

## Nota importante

Las instrucciones antiguas que dicen “Pitch CV como audio” están obsoletas para esta unidad. Usar:

```text
Mod CV / DAC2 / PA5 = audio
```
