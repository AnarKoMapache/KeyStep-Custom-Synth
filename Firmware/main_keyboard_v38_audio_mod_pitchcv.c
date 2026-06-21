/* USER CODE BEGIN Header */
/*
 * KeyStep Keyboard V38 - Audio por Mod + Pitch CV auxiliar
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
 * Arquitectura V38 según prueba real del usuario:
 *   La salida física que se escucha como audio es MOD CV.
 *   Mantengo el audio en el mismo canal que funcionó en V37.
 *   El otro DAC queda como Pitch CV auxiliar/voltaje de nota aproximado.
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

#define FM_SAMPLE_RATE_HZ  16000UL
#define FM_SAMPLE_PERIOD_US 62U

/* Envolventes 0..4096. V38: MultiEngine mono performance, sin arpegiador. */
#define MAX_POLY_VOICES       1U

/* V38: valores base; cada motor ajusta su comportamiento internamente. */
#define AMP_ATTACK_STEP       780U
#define AMP_DECAY_STEP        14U
#define AMP_SUSTAIN_LEVEL     2450U
#define AMP_RELEASE_STEP      20U

/* Reutilizamos filter_env como FM-index envelope: golpe brillante que luego se suaviza. */
#define FILTER_ATTACK_STEP    1050U
#define FILTER_DECAY_STEP     18U
#define FILTER_SUSTAIN_LEVEL  1100U
#define FILTER_RELEASE_STEP   34U

#define PORTAMENTO_SHIFT      5U    /* glide base */

#define ENGINE_MICROFREAK     0U
#define ENGINE_YAMAHA_FM      1U
#define ENGINE_MOOG_LEAD      2U
#define ENGINE_ACID_BASS      3U
#define ENGINE_DIGITAL_BELL   4U
#define ENGINE_COUNT          5U

/* Modo oculto:
 * Mantener la tecla más grave (key 31) + una de las 5 teclas siguientes:
 * key30 = MicroFreak-ish
 * key29 = Yamaha FM
 * key28 = Moog lead
 * key27 = Acid bass
 * key26 = Digital bell/pad
 * key25 = octava abajo
 * key24 = octava normal
 * key23 = octava arriba
 * key22 = timbre oscuro
 * key21 = timbre normal
 * key20 = timbre brillante
 * key19 = vibrato off
 * key18 = vibrato normal
 * key17 = vibrato profundo
 */
#define MODE_FUNC_KEY         31U
#define MODE_KEY_0            30U
#define MODE_KEY_1            29U
#define MODE_KEY_2            28U
#define MODE_KEY_3            27U
#define MODE_KEY_4            26U
#define MODE_KEY_OCT_DOWN     25U
#define MODE_KEY_OCT_RESET    24U
#define MODE_KEY_OCT_UP       23U
#define MODE_KEY_DARK         22U
#define MODE_KEY_NORMAL       21U
#define MODE_KEY_BRIGHT       20U
#define MODE_KEY_VIB_OFF      19U
#define MODE_KEY_VIB_NORMAL   18U
#define MODE_KEY_VIB_DEEP     17U

static uint8_t synth_mode = ENGINE_MICROFREAK;
static int8_t octave_shift = 0;  /* -1, 0, +1 */
static uint8_t tone_brightness = 1U; /* 0 oscuro, 1 normal, 2 brillante */
static uint8_t vibrato_depth = 1U;   /* 0 off, 1 normal, 2 profundo */



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
static int32_t fm_soft_clip(int32_t x);
static int32_t sin8_lookup(uint32_t phase);
static int32_t saw12_lookup(uint32_t phase);
static int32_t tri12_lookup(uint32_t phase);
static int32_t pulse_lookup(uint32_t phase, uint16_t width);
static int32_t wavefold(int32_t x);
static void Poly_Init(void);
static void Poly_KeyDown(uint8_t key_index);
static void Poly_KeyUp(uint8_t key_index);
static void Poly_HandleMaskChange(uint32_t old_mask, uint32_t new_mask);
static uint32_t KeyboardMatrix_KeyMask(const uint8_t rows[8]);
static void Poly_UpdateKeyboardDebounced(void);
static uint16_t Poly_RenderSample(void);
static uint16_t AuxPitchCV_Render(void);

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
  beep(660, 50);
  pause_ms(50);
  beep(990, 50);
  pause_ms(50);
  beep(1320, 70);
  pause_ms(100);

  if (any_key_now())
  {
    beep(180, 90);
    wait_release();
  }

  while (1)
  {
    Poly_UpdateKeyboardDebounced();
    Bare_DAC_Set(Poly_RenderSample(), AuxPitchCV_Render());
    delay_us(FM_SAMPLE_PERIOD_US);
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

static int32_t fm_soft_clip(int32_t x)
{
  /* Soft clip digital limpio: menos barro que la saturación Moog-ish. */
  if (x > 1800)  x = 1800 + (x - 1800) / 4;
  if (x < -1800) x = -1800 + (x + 1800) / 4;
  if (x > 2047)  x = 2047;
  if (x < -2047) x = -2047;
  return x;
}


static const int16_t sine256[256] = {
      0,    50,   100,   151,   201,   251,   300,   350,
    399,   449,   497,   546,   594,   642,   690,   737,
    783,   830,   875,   920,   965,  1009,  1052,  1095,
   1137,  1179,  1219,  1259,  1299,  1337,  1375,  1411,
   1447,  1483,  1517,  1550,  1582,  1614,  1644,  1674,
   1702,  1729,  1756,  1781,  1805,  1828,  1850,  1871,
   1891,  1910,  1927,  1944,  1959,  1973,  1986,  1997,
   2008,  2017,  2025,  2032,  2037,  2041,  2045,  2046,
   2047,  2046,  2045,  2041,  2037,  2032,  2025,  2017,
   2008,  1997,  1986,  1973,  1959,  1944,  1927,  1910,
   1891,  1871,  1850,  1828,  1805,  1781,  1756,  1729,
   1702,  1674,  1644,  1614,  1582,  1550,  1517,  1483,
   1447,  1411,  1375,  1337,  1299,  1259,  1219,  1179,
   1137,  1095,  1052,  1009,   965,   920,   875,   830,
    783,   737,   690,   642,   594,   546,   497,   449,
    399,   350,   300,   251,   201,   151,   100,    50,
      0,   -50,  -100,  -151,  -201,  -251,  -300,  -350,
   -399,  -449,  -497,  -546,  -594,  -642,  -690,  -737,
   -783,  -830,  -875,  -920,  -965, -1009, -1052, -1095,
  -1137, -1179, -1219, -1259, -1299, -1337, -1375, -1411,
  -1447, -1483, -1517, -1550, -1582, -1614, -1644, -1674,
  -1702, -1729, -1756, -1781, -1805, -1828, -1850, -1871,
  -1891, -1910, -1927, -1944, -1959, -1973, -1986, -1997,
  -2008, -2017, -2025, -2032, -2037, -2041, -2045, -2046,
  -2047, -2046, -2045, -2041, -2037, -2032, -2025, -2017,
  -2008, -1997, -1986, -1973, -1959, -1944, -1927, -1910,
  -1891, -1871, -1850, -1828, -1805, -1781, -1756, -1729,
  -1702, -1674, -1644, -1614, -1582, -1550, -1517, -1483,
  -1447, -1411, -1375, -1337, -1299, -1259, -1219, -1179,
  -1137, -1095, -1052, -1009,  -965,  -920,  -875,  -830,
   -783,  -737,  -690,  -642,  -594,  -546,  -497,  -449,
   -399,  -350,  -300,  -251,  -201,  -151,  -100,   -50,
};

static int32_t sin8_lookup(uint32_t phase)
{
  return (int32_t)sine256[(phase >> 24) & 0xFFU];
}

static int32_t saw12_lookup(uint32_t phase)
{
  /* -2048..2047 */
  return (int32_t)((phase >> 20) & 0x0FFFU) - 2048;
}

static int32_t tri12_lookup(uint32_t phase)
{
  uint32_t x = (phase >> 20) & 0x0FFFU;
  int32_t y = (x < 2048U) ? (int32_t)x : (int32_t)(4095U - x);
  return (y * 2) - 2048;
}

static int32_t pulse_lookup(uint32_t phase, uint16_t width)
{
  /* width 256..3840, salida más baja que square para no hacer barro. */
  uint16_t x = (uint16_t)((phase >> 20) & 0x0FFFU);
  if (width < 256U) width = 256U;
  if (width > 3840U) width = 3840U;
  return (x < width) ? 1450 : -1450;
}

static int32_t wavefold(int32_t x)
{
  /* Fold digital suave, tipo motor waveshaper, no distorsión dura. */
  const int32_t limit = 1900;
  while (x > limit || x < -limit)
  {
    if (x > limit)  x = limit - (x - limit);
    if (x < -limit) x = -limit - (x + limit);
  }
  return x;
}

static PolyVoice voices[MAX_POLY_VOICES];
static uint32_t stable_key_mask = 0U;
static uint32_t last_raw_key_mask = 0U;
static uint8_t raw_stable_count = 0U;
static uint8_t scan_divider = 0U;
static uint32_t micro_lfo_phase = 0U;

static uint32_t synth_target_inc = 0U;
static uint8_t synth_current_key = 255U;
static uint8_t synth_gate = 0U;

static uint8_t key_count(uint32_t mask)
{
  uint8_t count = 0U;
  for (uint8_t key = 0U; key < 32U; key++)
  {
    if ((mask & (1UL << key)) != 0U)
    {
      count++;
    }
  }
  return count;
}

static uint8_t first_key_from_mask(uint32_t mask)
{
  for (uint8_t key = 0U; key < 32U; key++)
  {
    if ((mask & (1UL << key)) != 0U)
    {
      return key;
    }
  }
  return 255U;
}

static uint8_t handle_mode_combo(uint32_t mask);

static uint8_t lowest_key_from_mask(uint32_t mask)
{
  /* key 31 es la nota mas grave segun nuestra tabla. */
  for (int8_t key = 31; key >= 0; key--)
  {
    if ((mask & (1UL << (uint8_t)key)) != 0U)
    {
      return (uint8_t)key;
    }
  }
  return 255U;
}

static uint32_t inc_for_key(uint8_t key_index)
{
  uint32_t freq_hz = note_freq_for_key(key_index);

  /* V38: cambio de octava desde combo oculto. Limitado para que el DAC no se vuelva puro aliasing. */
  if (octave_shift > 0)
  {
    for (int8_t i = 0; i < octave_shift; i++)
    {
      freq_hz *= 2U;
    }
  }
  else if (octave_shift < 0)
  {
    for (int8_t i = octave_shift; i < 0; i++)
    {
      freq_hz = (freq_hz > 2U) ? (freq_hz / 2U) : freq_hz;
    }
  }

  if (freq_hz < 35U) freq_hz = 35U;
  if (freq_hz > 1800U) freq_hz = 1800U;

  return (uint32_t)(((uint64_t)freq_hz << 32) / FM_SAMPLE_RATE_HZ);
}

static void synth_trigger_key(uint8_t key_index, uint8_t retrigger)
{
  if (key_index > 31U)
  {
    return;
  }

  PolyVoice *v = &voices[0];

  if (!v->active)
  {
    v->active = 1U;
    v->phase1 = 0U;
    v->phase2 = 0x40000000UL;
    v->phase_sub = 0x80000000UL;
    v->filter_state = 0;
    v->inc1 = inc_for_key(key_index);
  }

  synth_gate = 1U;
  v->held = 1U;
  v->key_index = key_index;
  synth_current_key = key_index;
  synth_target_inc = inc_for_key(key_index);

  if (retrigger)
  {
    /* Ataque digital: nota nueva con golpe claro y glide corto. */
    v->inc1 = synth_target_inc;
    if (v->amp_env > 500U) v->amp_env = 500U;
    v->filter_env = 0U;
  }
}

static void synth_release(void)
{
  synth_gate = 0U;
  voices[0].held = 0U;
}

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
  micro_lfo_phase = 0U;
  synth_target_inc = 0U;
  synth_current_key = 255U;
  synth_gate = 0U;
  octave_shift = 0;
  tone_brightness = 1U;
  vibrato_depth = 1U;
}

static void Poly_KeyDown(uint8_t key_index)
{
  synth_trigger_key(key_index, 1U);
}

static void Poly_KeyUp(uint8_t key_index)
{
  (void)key_index;
  synth_release();
}

static uint8_t handle_mode_combo(uint32_t mask)
{
  if ((mask & (1UL << MODE_FUNC_KEY)) == 0U)
  {
    return 0U;
  }

  uint8_t new_mode = 255U;

  if ((mask & (1UL << MODE_KEY_0)) != 0U) new_mode = ENGINE_MICROFREAK;
  if ((mask & (1UL << MODE_KEY_1)) != 0U) new_mode = ENGINE_YAMAHA_FM;
  if ((mask & (1UL << MODE_KEY_2)) != 0U) new_mode = ENGINE_MOOG_LEAD;
  if ((mask & (1UL << MODE_KEY_3)) != 0U) new_mode = ENGINE_ACID_BASS;
  if ((mask & (1UL << MODE_KEY_4)) != 0U) new_mode = ENGINE_DIGITAL_BELL;

  if (new_mode != 255U)
  {
    synth_mode = new_mode;
    synth_release();
    synth_current_key = 255U;
    voices[0].filter_state = 0;
    voices[0].filter_env = 0U;
    voices[0].amp_env = 0U;
    voices[0].phase1 = 0U;
    voices[0].phase2 = 0x40000000UL + ((uint32_t)new_mode << 24);
    voices[0].phase_sub = 0x80000000UL;
    return 1U;
  }

  /* V38: octavas desde la misma tecla FUNC. */
  int8_t new_octave = 99;
  if ((mask & (1UL << MODE_KEY_OCT_DOWN)) != 0U)  new_octave = -1;
  if ((mask & (1UL << MODE_KEY_OCT_RESET)) != 0U) new_octave = 0;
  if ((mask & (1UL << MODE_KEY_OCT_UP)) != 0U)    new_octave = 1;

  if (new_octave != 99)
  {
    octave_shift = new_octave;
    synth_release();
    synth_current_key = 255U;
    synth_target_inc = 0U;
    voices[0].filter_state = 0;
    voices[0].filter_env = 0U;
    voices[0].amp_env = 0U;
    return 1U;
  }

  if ((mask & (1UL << MODE_KEY_DARK)) != 0U)
  {
    tone_brightness = 0U;
    return 1U;
  }
  if ((mask & (1UL << MODE_KEY_NORMAL)) != 0U)
  {
    tone_brightness = 1U;
    return 1U;
  }
  if ((mask & (1UL << MODE_KEY_BRIGHT)) != 0U)
  {
    tone_brightness = 2U;
    return 1U;
  }

  if ((mask & (1UL << MODE_KEY_VIB_OFF)) != 0U)
  {
    vibrato_depth = 0U;
    return 1U;
  }
  if ((mask & (1UL << MODE_KEY_VIB_NORMAL)) != 0U)
  {
    vibrato_depth = 1U;
    return 1U;
  }
  if ((mask & (1UL << MODE_KEY_VIB_DEEP)) != 0U)
  {
    vibrato_depth = 2U;
    return 1U;
  }

  return 0U;
}

static void Poly_HandleMaskChange(uint32_t old_mask, uint32_t new_mask)
{
  if (handle_mode_combo(new_mask))
  {
    return;
  }

  uint8_t new_count = key_count(new_mask);

  if (new_count == 0U)
  {
    synth_release();
    synth_current_key = 255U;
    return;
  }

  /* Sin arpegiador: prioridad mono.
   * - Si aparece una tecla nueva, toca esa.
   * - Si se suelta la tecla actual y quedan otras, cae a la más grave sostenida.
   */
  uint32_t added = new_mask & ~old_mask;

  if (added != 0U)
  {
    uint8_t key = first_key_from_mask(added);
    synth_trigger_key(key, 1U);
    return;
  }

  if (synth_current_key > 31U || ((new_mask & (1UL << synth_current_key)) == 0U))
  {
    uint8_t key = lowest_key_from_mask(new_mask);
    synth_trigger_key(key, 0U);
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
  /* Escaneo cada ~6 ms para no cortar el audio. */
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

  uint16_t amp_attack = AMP_ATTACK_STEP;
  uint16_t amp_decay = AMP_DECAY_STEP;
  uint16_t amp_sustain = AMP_SUSTAIN_LEVEL;
  uint16_t amp_release = AMP_RELEASE_STEP;
  uint16_t f_attack = FILTER_ATTACK_STEP;
  uint16_t f_decay = FILTER_DECAY_STEP;
  uint16_t f_sustain = FILTER_SUSTAIN_LEVEL;
  uint16_t f_release = FILTER_RELEASE_STEP;
  uint8_t porta_shift = PORTAMENTO_SHIFT;

  switch (synth_mode)
  {
    case ENGINE_YAMAHA_FM:
      amp_attack = 950U; amp_decay = 12U; amp_sustain = 2200U; amp_release = 18U;
      f_attack = 1200U; f_decay = 32U; f_sustain = 650U; f_release = 45U;
      porta_shift = 7U;
      break;
    case ENGINE_MOOG_LEAD:
      amp_attack = 520U; amp_decay = 8U; amp_sustain = 3300U; amp_release = 16U;
      f_attack = 900U; f_decay = 12U; f_sustain = 1500U; f_release = 20U;
      porta_shift = 5U;
      break;
    case ENGINE_ACID_BASS:
      amp_attack = 1200U; amp_decay = 42U; amp_sustain = 1300U; amp_release = 42U;
      f_attack = 1600U; f_decay = 78U; f_sustain = 350U; f_release = 80U;
      porta_shift = 4U;
      break;
    case ENGINE_DIGITAL_BELL:
      amp_attack = 850U; amp_decay = 20U; amp_sustain = 900U; amp_release = 10U;
      f_attack = 1350U; f_decay = 26U; f_sustain = 1200U; f_release = 18U;
      porta_shift = 8U;
      break;
    case ENGINE_MICROFREAK:
    default:
      break;
  }

  if (synth_gate)
  {
    if (v->amp_env < 4096U)
    {
      v->amp_env = (uint16_t)(((uint32_t)v->amp_env + amp_attack > 4096U) ? 4096U : v->amp_env + amp_attack);
    }
    else if (v->amp_env > amp_sustain)
    {
      v->amp_env = (uint16_t)((v->amp_env > amp_decay + amp_sustain) ? v->amp_env - amp_decay : amp_sustain);
    }

    if (v->filter_env < 4096U)
    {
      v->filter_env = (uint16_t)(((uint32_t)v->filter_env + f_attack > 4096U) ? 4096U : v->filter_env + f_attack);
    }
    else if (v->filter_env > f_sustain)
    {
      v->filter_env = (uint16_t)((v->filter_env > f_decay + f_sustain) ? v->filter_env - f_decay : f_sustain);
    }
  }
  else
  {
    v->amp_env = (uint16_t)((v->amp_env > amp_release) ? v->amp_env - amp_release : 0U);
    v->filter_env = (uint16_t)((v->filter_env > f_release) ? v->filter_env - f_release : 0U);

    if (v->amp_env == 0U)
    {
      v->active = 0U;
      return 0;
    }
  }

  if (synth_target_inc != 0U)
  {
    if (v->inc1 == 0U)
    {
      v->inc1 = synth_target_inc;
    }
    else if (v->inc1 != synth_target_inc)
    {
      int32_t diff = (int32_t)(synth_target_inc - v->inc1);
      int32_t step = diff >> porta_shift;
      if (step == 0)
      {
        step = (diff > 0) ? 1 : -1;
      }
      v->inc1 = (uint32_t)((int32_t)v->inc1 + step);
    }
  }

  micro_lfo_phase += 75000UL + ((uint32_t)synth_mode * 17000UL);
  int32_t lfo = sin8_lookup(micro_lfo_phase);
  uint32_t env = (uint32_t)v->filter_env;

  /* V38: macros de performance. Brillo controla el indice FM / apertura de filtro. */
  if (tone_brightness == 0U)
  {
    env = (env * 62U) / 100U;
  }
  else if (tone_brightness == 2U)
  {
    env = (env * 145U) / 100U;
    if (env > 4096U) env = 4096U;
  }

  /* Vibrato digital liviano: no intenta ser perfecto, solo da movimiento tipo performance. */
  uint32_t phase_inc = v->inc1;
  if (synth_gate && vibrato_depth != 0U)
  {
    int32_t vib = (vibrato_depth == 1U) ? (lfo / 1500) : (lfo / 650);
    if (vib != 0)
    {
      int32_t delta = ((int32_t)(v->inc1 >> 9) * vib) / 16;
      phase_inc = (uint32_t)((int32_t)v->inc1 + delta);
    }
  }

  int32_t x = 0;

  switch (synth_mode)
  {
    case ENGINE_YAMAHA_FM:
    {
      /* Yamaha-ish: FM limpia, ataque brillante, cuerpo suave. */
      v->inc2 = v->inc1 * 2U;
      v->inc_sub = v->inc1 * 3U;
      int32_t mod1 = sin8_lookup(v->phase2);
      int32_t mod2 = sin8_lookup(v->phase_sub + ((uint32_t)lfo << 12));
      int32_t index = 45000 + (int32_t)(env * 230U);
      int64_t off = ((int64_t)mod1 * index) + ((int64_t)mod2 * (index / 5));
      int32_t car = sin8_lookup(v->phase1 + (uint32_t)off);
      int32_t body = sin8_lookup((v->phase1 >> 1) + (uint32_t)(off / 8));
      x = (car * 5 + body * 2) / 7;
      break;
    }

    case ENGINE_MOOG_LEAD:
    {
      /* Moog-ish: sierra + pulso suave, filtro digital simple. */
      v->inc2 = v->inc1 + (v->inc1 / 240U);
      v->inc_sub = v->inc1 / 2U;
      int32_t raw = (saw12_lookup(v->phase1) * 5 + pulse_lookup(v->phase2, (uint16_t)(1900 + (lfo / 4))) * 2 + tri12_lookup(v->phase_sub)) / 8;
      uint8_t shift = (env > 2600U) ? 2U : ((env > 1200U) ? 3U : 4U);
      if (tone_brightness == 0U && shift < 5U) shift++;
      if (tone_brightness == 2U && shift > 1U) shift--;
      v->filter_state += (raw - v->filter_state) >> shift;
      x = v->filter_state;
      break;
    }

    case ENGINE_ACID_BASS:
    {
      /* Bass ácido: más nasal, decay rápido y cuerpo mono. */
      v->inc2 = v->inc1;
      v->inc_sub = v->inc1 / 2U;
      int32_t raw = (saw12_lookup(v->phase1) * 6 + pulse_lookup(v->phase2, 1350U) * 3 + tri12_lookup(v->phase_sub)) / 10;
      int32_t bite = wavefold((raw * (1200 + (int32_t)(env / 3U))) / 900);
      uint8_t shift = (env > 1800U) ? 2U : 4U;
      if (tone_brightness == 0U && shift < 5U) shift++;
      if (tone_brightness == 2U && shift > 1U) shift--;
      v->filter_state += (bite - v->filter_state) >> shift;
      x = v->filter_state;
      break;
    }

    case ENGINE_DIGITAL_BELL:
    {
      /* Digital bell/pad: FM inarmónica suave, cola más fría. */
      v->inc2 = v->inc1 * 4U;
      v->inc_sub = v->inc1 * 7U;
      int32_t mod1 = sin8_lookup(v->phase2);
      int32_t mod2 = sin8_lookup(v->phase_sub);
      int64_t off = ((int64_t)mod1 * (26000 + (int32_t)env * 145)) + ((int64_t)mod2 * 38000);
      int32_t car = sin8_lookup(v->phase1 + (uint32_t)off);
      int32_t shimmer = sin8_lookup((v->phase1 * 3U) + (uint32_t)(off / 4));
      x = (car * 4 + shimmer) / 5;
      break;
    }

    case ENGINE_MICROFREAK:
    default:
    {
      /* MicroFreak-ish: wavetable + FM + wavefolding con movimiento. */
      v->inc2 = v->inc1 * 2U;
      v->inc_sub = v->inc1 * 5U;
      int32_t mod1 = sin8_lookup(v->phase2 + ((uint32_t)v->filter_state << 8));
      int32_t mod2 = sin8_lookup(v->phase_sub);
      v->filter_state = (v->filter_state * 5 + mod1) / 6;
      int32_t mod_mix = (mod1 * 5 + mod2 * 2 + v->filter_state) / 8;
      int32_t index_main = 75000 + (int32_t)(env * 330U);
      int64_t phase_offset = ((int64_t)mod_mix * index_main) + ((int64_t)lfo * 22000);
      int32_t carrier = sin8_lookup(v->phase1 + (uint32_t)phase_offset);
      int32_t folded = wavefold((saw12_lookup(v->phase1 + ((uint32_t)lfo << 12)) + carrier) / 2);
      x = (carrier * 4 + folded * 2 + tri12_lookup(v->phase_sub)) / 7;
      break;
    }
  }

  v->phase1 += phase_inc;
  v->phase2 += v->inc2;
  v->phase_sub += v->inc_sub;

  x = fm_soft_clip(x);
  return (x * (int32_t)v->amp_env) >> 13;
}

static uint16_t Poly_RenderSample(void)
{
  int32_t mix = Poly_RenderVoice(&voices[0]);

  mix = fm_soft_clip(mix);

  int32_t out = (int32_t)DAC_MID + mix;
  if (out < 160) out = 160;
  if (out > 3935) out = 3935;
  return (uint16_t)out;
}

static uint16_t AuxPitchCV_Render(void)
{
  /* V38:
   * El audio queda en el mismo DAC que funcionó en V37.
   * Este segundo DAC entrega un voltaje de nota aproximado, útil como CV auxiliar.
   * No es audio: debería cambiar de voltaje al cambiar de tecla.
   */
  static uint16_t last_cv = 520U;

  if (synth_current_key <= 31U)
  {
    /* Nuestra tabla tiene key31 como grave y key0 como agudo.
     * Usamos ~100 cuentas por semitono, cercano a 1V/oct en DAC 3.3V.
     */
    int16_t semis = (int16_t)(31U - synth_current_key) + ((int16_t)octave_shift * 12);
    int32_t cv = 520 + ((int32_t)semis * 100);

    if (cv < 120) cv = 120;
    if (cv > 3950) cv = 3950;
    last_cv = (uint16_t)cv;
  }

  return last_cv;
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
