# KeyStep V34B MicroFreak-ish No ARP

Corrección del V34: eliminada una definición duplicada de `Poly_RenderVoice()` que provocaba errores de compilación y referencias antes de declaración (`synth_gate`, `synth_target_inc`, `micro_lfo_phase`).

- Matriz real: PE8-PE15 selección, PD8-PD15 lectura.
- Sin arpegiador.
- Monofónico.
- Audio por Pitch CV / DAC1 / PA4.
- Mod CV / DAC2 / PA5 centrado.

No conectar audífonos directo al CV. Usar mixer, interfaz o entrada de sinte con volumen bajo.
