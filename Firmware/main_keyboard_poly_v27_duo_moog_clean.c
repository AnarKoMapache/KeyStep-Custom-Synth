/* USER CODE BEGIN Header */
/*
 * KeyStep Keyboard Note V27 - Duo Moog Clean Real Matrix PE/PD
 *
 * Basado en reverse engineering del firmware oficial:
 *
 *   Select lines: PE8..PE15, activas en bajo
 *   Read lines:   PD8..PD15, activas en bajo
 *
 * Matriz:
 *   PE8  = contacto A grupo 0, lee PD8..PD15
 *   PE9  = contacto B grupo 0, lee PD8..PD15
 *   PE10 = contacto A grupo 1, lee PD8..PD15
 *   PE11 = contacto B grupo 1, lee PD8..PD15
 *   PE12 = contacto A grupo 2, lee PD8..PD15
 *   PE13 = contacto B grupo 2, lee PD8..PD15
 *   PE14 = contacto A grupo 3, lee PD8..PD15
 *   PE15 = contacto B grupo 3, lee PD8..PD15
 *
 * 32 teclas x 2 contactos = 64 contactos = 8 x 8.
 *
 * Audio de prueba:
 *   DAC1 / PA4 / Pitch CV = sintetizador DUO/parafonico limpio estilo Moog, menos masa.
 *   DAC2 / PA5 / Mod CV   = queda centrado.
 *
 * No conectar audifonos directo al CV. Usar entrada de sinte/mixer/interfaz con volumen bajo.
 */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN PV */

#define APP_BASE_ADDRESS 0x08004000UL

#define RCC_APB2ENR_REG      (*(volatile uint32_t*)0x40021018UL)
#define RCC_APB1ENR_REG      (*(volatile uint32_t*)0x4002101CUL)

#define RCC_APB2ENR_IOPAEN   (1UL << 2)
#define RCC_APB2ENR_IOPDEN   (1UL << 5)
#define RCC_APB2ENR_IOPEEN   (1UL << 6)
#define RCC_APB1ENR_DACEN    (1UL << 29)

#define GPIOA_BASE_ADDR      0x40010800UL
#define GPIOD_BASE_ADDR      0x40011400UL
#define GPIOE_BASE_ADDR      0x40011800UL

#define GPIO_CRL(base)       (*(volatile uint32_t*)((base) + 0x00UL))
#define GPIO_CRH(base)       (*(volatile uint32_t*)((base) + 0x04UL))
#define GPIO_IDR(base)       (*(volatile uint32_t*)((base) + 0x08UL))
#define GPIO_ODR(base)       (*(volatile uint32_t*)((base) + 0x0CUL))

#define DAC_CR_REG           (*(volatile uint32_t*)0x40007400UL)
#define DAC_DHR12R1_REG      (*(volatile uint32_t*)0x40007408UL)
#define DAC_DHR12R2_REG      (*(volatile uint32_t*)0x40007414UL)

#define DAC_CR_EN1           (1UL << 0)
#define DAC_CR_EN2           (1UL << 16)

#define DAC_LOW              180U
#define DAC_MID              2048U
#define DAC_HIGH             3920U

#define SELECT_MASK          0xFF00U  /* PE8..PE15 */
#define READ_MASK            0xFF00U  /* PD8..PD15 */

#define CONTACT_A            0x01U
#define CONTACT_B            0x02U

#define MOOG_SAMPLE_RATE_HZ  16000UL
#define MOOG_SAMPLE_PERIOD_US 62U

/* Envolventes 0..4096. Ataque rápido, decay lento y sustain medio: más tipo sinte analógico. */
#define MAX_POLY_VOICES       2U

#define AMP_ATTACK_STEP       190U
#define AMP_DECAY_STEP        8U
#define AMP_SUSTAIN_LEVEL     3000U
#define AMP_RELEASE_STEP      42U

#define FILTER_ATTACK_STEP    160U
#define FILTER_DECAY_STEP     18U
#define FILTER_SUSTAIN_LEVEL  650U
#define FILTER_RELEASE_STEP   44U


/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */
static void DWT_Init_Counter(void);
static void delay_us(uint32_t us);
static void Bare_DAC_Init(void);
static void Bare_DAC_Set(uint16_t ch1, uint16_t ch2);
static void beep(uint32_t freq_hz, uint32_t duration_ms);
static void pause_ms(uint32_t ms);
static void beep_count(uint8_t count, uint32_t freq_hz);
static void report_start(void);
static void report_ready(void);


typedef struct
{
  uint8_t active;
  uint8_t held;
  uint8_t key_index;
  uint8_t age;
  uint32_t phase1;
  uint32_t phase2;
  uint32_t phase_sub;
  uint32_t inc1;
  uint32_t inc2;
  uint32_t inc_sub;
  uint16_t amp_env;
  uint16_t filter_env;
  int32_t filter_state;
} PolyVoice;

typedef struct
{
  uint8_t valid;
  uint8_t key_index;
  uint8_t contact_mask;
} KeyEvent;

static uint32_t note_freq_for_key(uint8_t key_index);
static int32_t moog_soft_clip(int32_t x);
static void Poly_Init(void);
static void Poly_KeyDown(uint8_t key_index);
static void Poly_KeyUp(uint8_t key_index);
static void Poly_HandleMaskChange(uint32_t old_mask, uint32_t new_mask);
static uint32_t KeyboardMatrix_KeyMask(const uint8_t rows[8]);
static void Poly_UpdateKeyboardDebounced(void);
static int32_t Poly_RenderVoice(PolyVoice *v);
static uint16_t Poly_RenderSample(void);

static void gpio_set_pin_mode(uint32_t base, uint8_t pin, uint8_t mode_cnf);
static void gpio_input_pullup(uint32_t base, uint8_t pin);
static void gpio_output_od_high(uint32_t base, uint8_t pin);
static void KeyboardMatrix_GPIO_Init(void);
static uint8_t KeyboardMatrix_ScanRow(uint8_t row);
static void KeyboardMatrix_ScanRows(uint8_t rows[8]);
static KeyEvent KeyboardMatrix_FindFirstKey(const uint8_t rows[8]);
static KeyEvent KeyboardMatrix_ScanStable(void);
static uint8_t same_event(KeyEvent a, KeyEvent b);
static uint8_t any_key_now(void);
static void wait_release(void);

/* USER CODE END PFP */

int main(void)
{
  SCB->VTOR = APP_BASE_ADDRESS;
  __DSB();
  __ISB();

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  DWT_Init_Counter();
  Bare_DAC_Init();
  Bare_DAC_Set(DAC_MID, DAC_MID);
  KeyboardMatrix_GPIO_Init();
  Poly_Init();

  HAL_Delay(500);
  beep(880, 55);
  pause_ms(70);
  beep(1320, 55);
  pause_ms(120);

  if (any_key_now())
  {
    beep(180, 90);
    wait_release();
  }

  while (1)
  {
    Poly_UpdateKeyboardDebounced();
    Bare_DAC_Set(Poly_RenderSample(), DAC_MID);
    delay_us(MOOG_SAMPLE_PERIOD_US);
  }
}

/* USER CODE BEGIN 4 */

static void DWT_Init_Counter(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
  uint32_t cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000UL;
  uint32_t cycles = us * cycles_per_us;
  uint32_t start = DWT->CYCCNT;

  while ((uint32_t)(DWT->CYCCNT - start) < cycles)
  {
    __NOP();
  }
}

static void Bare_DAC_Init(void)
{
  RCC_APB2ENR_REG |= RCC_APB2ENR_IOPAEN;
  RCC_APB1ENR_REG |= RCC_APB1ENR_DACEN;

  /* PA4 / PA5 analog para DAC. */
  GPIO_CRL(GPIOA_BASE_ADDR) &= ~((0xFUL << 16) | (0xFUL << 20));

  DAC_CR_REG = 0;
  DAC_CR_REG |= DAC_CR_EN1 | DAC_CR_EN2;
}

static void Bare_DAC_Set(uint16_t ch1, uint16_t ch2)
{
  if (ch1 > 4095U) ch1 = 4095U;
  if (ch2 > 4095U) ch2 = 4095U;
  DAC_DHR12R1_REG = ch1;
  DAC_DHR12R2_REG = ch2;
}

static void beep(uint32_t freq_hz, uint32_t duration_ms)
{
  if (freq_hz == 0U)
  {
    HAL_Delay(duration_ms);
    return;
  }

  uint32_t half_period_us = 500000UL / freq_hz;
  if (half_period_us < 20U) half_period_us = 20U;

  uint32_t cycles = (duration_ms * freq_hz) / 1000UL;
  if (cycles < 1U) cycles = 1U;

  for (uint32_t i = 0; i < cycles; i++)
  {
    Bare_DAC_Set(DAC_HIGH, DAC_MID);
    delay_us(half_period_us);
    Bare_DAC_Set(DAC_LOW, DAC_MID);
    delay_us(half_period_us);
  }

  Bare_DAC_Set(DAC_MID, DAC_MID);
}

static void pause_ms(uint32_t ms)
{
  Bare_DAC_Set(DAC_MID, DAC_MID);
  HAL_Delay(ms);
}

static void beep_count(uint8_t count, uint32_t freq_hz)
{
  for (uint8_t i = 0U; i < count; i++)
  {
    beep(freq_hz, 45U);
    pause_ms(45U);
  }
}

static void report_start(void)
{
  beep(440U, 70U); pause_ms(50U);
  beep(660U, 70U); pause_ms(50U);
}

static void report_ready(void)
{
  beep(880U, 70U); pause_ms(50U);
  beep(1100U, 90U); pause_ms(80U);
}

static uint32_t note_freq_for_key(uint8_t key_index)
{
  /*
   * Mapa directo basado en la fórmula observada en el firmware oficial:
   *   nota aproximada = 72 - keyIndex
   *
   * key 0  = C5  aprox 523 Hz
   * key 31 = F2  aprox  87 Hz
   */
  static const uint16_t key_freq_hz[32] = {
    523, 494, 466, 440, 415, 392, 370, 349,
    330, 311, 294, 277, 262, 247, 233, 220,
    208, 196, 185, 175, 165, 156, 147, 139,
    131, 123, 117, 110, 104,  98,  92,  87
  };

  if (key_index > 31U)
  {
    key_index = 31U;
  }

  return key_freq_hz[key_index];
}

static int32_t moog_soft_clip(int32_t x)
{
  /* Saturación simple para que la mezcla polifónica no rompa tan feo. */
  if (x > 1500)  x = 1500 + (x - 1500) / 5;
  if (x < -1500) x = -1500 + (x + 1500) / 5;
  if (x > 2047)  x = 2047;
  if (x < -2047) x = -2047;
  return x;
}

static PolyVoice voices[MAX_POLY_VOICES];
static uint8_t voice_age_counter = 0U;
static uint32_t stable_key_mask = 0U;
static uint32_t last_raw_key_mask = 0U;
static uint8_t raw_stable_count = 0U;
static uint8_t scan_divider = 0U;

static void Poly_Init(void)
{
  for (uint8_t i = 0U; i < MAX_POLY_VOICES; i++)
  {
    voices[i].active = 0U;
    voices[i].held = 0U;
    voices[i].key_index = 0U;
    voices[i].age = 0U;
    voices[i].phase1 = 0U;
    voices[i].phase2 = 0U;
    voices[i].phase_sub = 0U;
    voices[i].inc1 = 0U;
    voices[i].inc2 = 0U;
    voices[i].inc_sub = 0U;
    voices[i].amp_env = 0U;
    voices[i].filter_env = 0U;
    voices[i].filter_state = 0;
  }

  stable_key_mask = 0U;
  last_raw_key_mask = 0U;
  raw_stable_count = 0U;
  scan_divider = 0U;
}

static void Poly_KeyDown(uint8_t key_index)
{
  /* Si la voz ya existe en release, reactivarla. */
  for (uint8_t i = 0U; i < MAX_POLY_VOICES; i++)
  {
    if (voices[i].active && voices[i].key_index == key_index)
    {
      voices[i].held = 1U;
      voices[i].age = voice_age_counter++;
      return;
    }
  }

  uint8_t chosen = 0U;
  uint8_t found_free = 0U;

  for (uint8_t i = 0U; i < MAX_POLY_VOICES; i++)
  {
    if (!voices[i].active)
    {
      chosen = i;
      found_free = 1U;
      break;
    }
  }

  if (!found_free)
  {
    /* Robar la voz más antigua. */
    uint8_t oldest_age = voices[0].age;
    chosen = 0U;
    for (uint8_t i = 1U; i < MAX_POLY_VOICES; i++)
    {
      if (voices[i].age < oldest_age)
      {
        oldest_age = voices[i].age;
        chosen = i;
      }
    }
  }

  uint32_t freq_hz = note_freq_for_key(key_index);
  uint32_t inc1 = (uint32_t)(((uint64_t)freq_hz << 32) / MOOG_SAMPLE_RATE_HZ);

  voices[chosen].active = 1U;
  voices[chosen].held = 1U;
  voices[chosen].key_index = key_index;
  voices[chosen].age = voice_age_counter++;
  voices[chosen].phase1 = 0U;
  voices[chosen].phase2 = 0x12345678UL + ((uint32_t)key_index * 0x01020304UL);
  voices[chosen].phase_sub = 0x80000000UL;
  voices[chosen].inc1 = inc1;
  voices[chosen].inc2 = inc1;  /* V27: sin desafinación para que el acorde no se ensucie */
  voices[chosen].inc_sub = inc1 / 2U; /* no usado como sub audible en V27 */
  voices[chosen].amp_env = 0U;
  voices[chosen].filter_env = 0U;
  voices[chosen].filter_state = 0;
}

static void Poly_KeyUp(uint8_t key_index)
{
  for (uint8_t i = 0U; i < MAX_POLY_VOICES; i++)
  {
    if (voices[i].active && voices[i].key_index == key_index)
    {
      voices[i].held = 0U;
    }
  }
}

static void Poly_HandleMaskChange(uint32_t old_mask, uint32_t new_mask)
{
  uint32_t pressed = new_mask & ~old_mask;
  uint32_t released = old_mask & ~new_mask;

  for (uint8_t key = 0U; key < 32U; key++)
  {
    uint32_t bit = 1UL << key;
    if ((released & bit) != 0U)
    {
      Poly_KeyUp(key);
    }
  }

  for (uint8_t key = 0U; key < 32U; key++)
  {
    uint32_t bit = 1UL << key;
    if ((pressed & bit) != 0U)
    {
      Poly_KeyDown(key);
    }
  }
}

static uint32_t KeyboardMatrix_KeyMask(const uint8_t rows[8])
{
  uint32_t mask = 0U;

  for (uint8_t key = 0U; key < 32U; key++)
  {
    uint8_t group = key / 8U;
    uint8_t col = key % 8U;
    uint8_t bit = (uint8_t)(1U << col);

    if (((rows[group * 2U] & bit) != 0U) || ((rows[group * 2U + 1U] & bit) != 0U))
    {
      mask |= (1UL << key);
    }
  }

  return mask;
}

static void Poly_UpdateKeyboardDebounced(void)
{
  /* No escanear en cada muestra para no cortar demasiado el audio. */
  scan_divider++;
  if (scan_divider < 96U)
  {
    return;
  }
  scan_divider = 0U;

  uint8_t rows[8];
  KeyboardMatrix_ScanRows(rows);
  uint32_t raw = KeyboardMatrix_KeyMask(rows);

  if (raw == last_raw_key_mask)
  {
    if (raw_stable_count < 255U)
    {
      raw_stable_count++;
    }
  }
  else
  {
    last_raw_key_mask = raw;
    raw_stable_count = 0U;
  }

  if (raw_stable_count >= 1U && raw != stable_key_mask)
  {
    uint32_t old_mask = stable_key_mask;
    stable_key_mask = raw;
    Poly_HandleMaskChange(old_mask, stable_key_mask);
  }
}

static int32_t Poly_RenderVoice(PolyVoice *v)
{
  if (!v->active)
  {
    return 0;
  }

  if (v->held)
  {
    if (v->amp_env < 4096U)
    {
      v->amp_env = (uint16_t)(((uint32_t)v->amp_env + AMP_ATTACK_STEP > 4096U) ? 4096U : v->amp_env + AMP_ATTACK_STEP);
    }
    else if (v->amp_env > AMP_SUSTAIN_LEVEL)
    {
      v->amp_env = (uint16_t)((v->amp_env > AMP_DECAY_STEP + AMP_SUSTAIN_LEVEL) ? v->amp_env - AMP_DECAY_STEP : AMP_SUSTAIN_LEVEL);
    }

    if (v->filter_env < 4096U)
    {
      v->filter_env = (uint16_t)(((uint32_t)v->filter_env + FILTER_ATTACK_STEP > 4096U) ? 4096U : v->filter_env + FILTER_ATTACK_STEP);
    }
    else if (v->filter_env > FILTER_SUSTAIN_LEVEL)
    {
      v->filter_env = (uint16_t)((v->filter_env > FILTER_DECAY_STEP + FILTER_SUSTAIN_LEVEL) ? v->filter_env - FILTER_DECAY_STEP : FILTER_SUSTAIN_LEVEL);
    }
  }
  else
  {
    v->amp_env = (uint16_t)((v->amp_env > AMP_RELEASE_STEP) ? v->amp_env - AMP_RELEASE_STEP : 0U);
    v->filter_env = (uint16_t)((v->filter_env > FILTER_RELEASE_STEP) ? v->filter_env - FILTER_RELEASE_STEP : 0U);

    if (v->amp_env == 0U)
    {
      v->active = 0U;
      return 0;
    }
  }

  /* V27: menos masa.
   * Sacamos sub-oscilador y desafinación fuerte. Esto deja el acorde claro.
   * Mezcla: sierra suave + triángulo, después filtro pasa-bajos simple.
   */
  uint32_t p = (v->phase1 >> 20) & 0xFFFU;      /* 0..4095 */
  int32_t saw = (int32_t)p - 2048;              /* -2048..2047 */
  int32_t tri;

  if (p < 2048U)
  {
    tri = ((int32_t)p * 2) - 2048;
  }
  else
  {
    tri = 6143 - ((int32_t)p * 2);
  }

  int32_t pulse = ((v->phase1 & 0x80000000UL) != 0U) ? 650 : -650;

  v->phase1 += v->inc1;
  v->phase2 += v->inc2;
  v->phase_sub += v->inc_sub;

  int32_t x = (saw * 3 + tri * 6 + pulse) / 12;

  /* Cutoff más controlado: menos barro en acordes. */
  uint16_t cutoff = (uint16_t)(16U + (v->filter_env / 72U));
  if (cutoff > 78U) cutoff = 78U;
  if (cutoff < 10U) cutoff = 10U;

  v->filter_state += ((x - v->filter_state) * (int32_t)cutoff) >> 8;

  int32_t y = v->filter_state;
  y = (y * (int32_t)v->amp_env) >> 13;
  return y;
}

static uint16_t Poly_RenderSample(void)
{
  int32_t mix = 0;
  uint8_t active_count = 0U;

  for (uint8_t i = 0U; i < MAX_POLY_VOICES; i++)
  {
    if (voices[i].active)
    {
      active_count++;
    }
    mix += Poly_RenderVoice(&voices[i]);
  }

  /* V27: normaliza la mezcla para que los acordes no se vuelvan una masa. */
  if (active_count > 1U)
  {
    mix = (mix * 3) / 5;
  }

  mix = moog_soft_clip(mix);

  int32_t out = (int32_t)DAC_MID + mix;
  if (out < 160) out = 160;
  if (out > 3935) out = 3935;
  return (uint16_t)out;
}

/* GPIO mode_cnf STM32F1:
 * 0x8 = input pull-up / pull-down
 * 0x6 = output open-drain 2 MHz
 */
static void gpio_set_pin_mode(uint32_t base, uint8_t pin, uint8_t mode_cnf)
{
  volatile uint32_t *cr;
  uint8_t shift;

  if (pin < 8U)
  {
    cr = (volatile uint32_t*)(base + 0x00UL);
    shift = (uint8_t)(pin * 4U);
  }
  else
  {
    cr = (volatile uint32_t*)(base + 0x04UL);
    shift = (uint8_t)((pin - 8U) * 4U);
  }

  uint32_t v = *cr;
  v &= ~(0xFUL << shift);
  v |= ((uint32_t)mode_cnf << shift);
  *cr = v;
}

static void gpio_input_pullup(uint32_t base, uint8_t pin)
{
  GPIO_ODR(base) |= (1UL << pin);
  gpio_set_pin_mode(base, pin, 0x8U);
}

static void gpio_output_od_high(uint32_t base, uint8_t pin)
{
  GPIO_ODR(base) |= (1UL << pin);
  gpio_set_pin_mode(base, pin, 0x6U);
}

static void KeyboardMatrix_GPIO_Init(void)
{
  RCC_APB2ENR_REG |= RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN;

  /* PE8..PE15: select outputs, activos en bajo, quedan inactivos HIGH. */
  for (uint8_t pin = 8U; pin <= 15U; pin++)
  {
    gpio_output_od_high(GPIOE_BASE_ADDR, pin);
  }

  /* PD8..PD15: columnas/lecturas con pull-up. */
  for (uint8_t pin = 8U; pin <= 15U; pin++)
  {
    gpio_input_pullup(GPIOD_BASE_ADDR, pin);
  }

  GPIO_ODR(GPIOE_BASE_ADDR) |= SELECT_MASK;
}

static uint8_t KeyboardMatrix_ScanRow(uint8_t row)
{
  row &= 7U;

  /* Todas las selecciones inactivas. */
  GPIO_ODR(GPIOE_BASE_ADDR) |= SELECT_MASK;
  delay_us(6);

  /* Activar una seleccion en LOW. */
  GPIO_ODR(GPIOE_BASE_ADDR) &= ~(1UL << (8U + row));
  delay_us(28);

  /* Leer PD8..PD15 activo en bajo. */
  uint8_t value = (uint8_t)(~(GPIO_IDR(GPIOD_BASE_ADDR) >> 8) & 0xFFU);

  /* Dejar inactivo. */
  GPIO_ODR(GPIOE_BASE_ADDR) |= SELECT_MASK;

  return value;
}

static void KeyboardMatrix_ScanRows(uint8_t rows[8])
{
  for (uint8_t row = 0U; row < 8U; row++)
  {
    rows[row] = KeyboardMatrix_ScanRow(row);
  }
}

static KeyEvent KeyboardMatrix_FindFirstKey(const uint8_t rows[8])
{
  KeyEvent ev;
  ev.valid = 0U;
  ev.key_index = 0U;
  ev.contact_mask = 0U;

  for (uint8_t key = 0U; key < 32U; key++)
  {
    uint8_t group = key / 8U;
    uint8_t col = key % 8U;
    uint8_t bit = (uint8_t)(1U << col);
    uint8_t mask = 0U;

    if ((rows[group * 2U] & bit) != 0U)      mask |= CONTACT_A;
    if ((rows[group * 2U + 1U] & bit) != 0U)  mask |= CONTACT_B;

    if (mask != 0U)
    {
      ev.valid = 1U;
      ev.key_index = key;
      ev.contact_mask = mask;
      return ev;
    }
  }

  return ev;
}

static uint8_t same_event(KeyEvent a, KeyEvent b)
{
  return (a.valid && b.valid &&
          a.key_index == b.key_index &&
          a.contact_mask == b.contact_mask);
}

static KeyEvent KeyboardMatrix_ScanStable(void)
{
  KeyEvent last = {0U, 0U, 0U};
  uint8_t stable_count = 0U;

  while (1)
  {
    uint8_t rows[8];
    KeyboardMatrix_ScanRows(rows);
    KeyEvent now = KeyboardMatrix_FindFirstKey(rows);

    if (same_event(now, last))
    {
      if (stable_count < 255U)
      {
        stable_count++;
      }
    }
    else
    {
      last = now;
      stable_count = now.valid ? 1U : 0U;
    }

    if (now.valid && stable_count >= 5U)
    {
      return now;
    }

    HAL_Delay(7);
  }
}

static uint8_t any_key_now(void)
{
  uint8_t rows[8];
  KeyboardMatrix_ScanRows(rows);
  KeyEvent ev = KeyboardMatrix_FindFirstKey(rows);
  return ev.valid ? 1U : 0U;
}

static void wait_release(void)
{
  uint8_t clear_count = 0U;

  while (clear_count < 10U)
  {
    if (any_key_now())
    {
      clear_count = 0U;
    }
    else
    {
      clear_count++;
    }

    HAL_Delay(20);
  }
}

/* USER CODE END 4 */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  RCC_APB2ENR_REG |= RCC_APB2ENR_IOPAEN |
                     RCC_APB2ENR_IOPDEN |
                     RCC_APB2ENR_IOPEEN;
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
