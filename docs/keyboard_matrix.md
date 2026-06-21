# Keyboard matrix

## Pinout lógico

```text
Select lines: PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15
Read lines:   PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15
Logic: active-low
```

## Escaneo básico

```c
static uint8_t scan_row(uint8_t row)
{
    GPIOE->ODR |= 0xFF00;              // todas inactivas HIGH
    GPIOE->ODR &= ~(1u << (8 + row));  // una fila activa LOW

    for (volatile int i = 0; i < 80; i++) {}

    return (uint8_t)(~(GPIOD->IDR >> 8) & 0xFF);
}
```

## Mapeo de contactos

```text
Grupo de 8 teclas 0:
  contacto A → PE8  lee PD8–PD15
  contacto B → PE9  lee PD8–PD15

Grupo de 8 teclas 1:
  contacto A → PE10 lee PD8–PD15
  contacto B → PE11 lee PD8–PD15

Grupo de 8 teclas 2:
  contacto A → PE12 lee PD8–PD15
  contacto B → PE13 lee PD8–PD15

Grupo de 8 teclas 3:
  contacto A → PE14 lee PD8–PD15
  contacto B → PE15 lee PD8–PD15
```

Para `keyIndex` de 0 a 31:

```text
grupo   = keyIndex / 8
columna = keyIndex % 8

Contacto A:
  select = PE(8 + grupo*2)
  read   = PD(8 + columna)

Contacto B:
  select = PE(8 + grupo*2 + 1)
  read   = PD(8 + columna)
```
