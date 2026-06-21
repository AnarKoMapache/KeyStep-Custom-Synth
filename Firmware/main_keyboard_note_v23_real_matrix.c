/* USER CODE BEGIN Header */
/*
 * KeyStep Keyboard Scan V23 - Real Matrix Direct Notes PE/PD
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
 *   DAC1 / PA4 / Pitch CV = una nota continua por tecla mientras se mantiene presionada.
 *   DAC2 / PA5 / Mod CV   = queda centrado; no se usa para codificar beeps.
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
static uint32_t note_freq_for_key(uint8_t key_index);
static void play_note_while_held(uint8_t key_index);

static void gpio_set_pin_mode(uint32_t base, uint8_t pin, uint8_t mode_cnf);
static void gpio_input_pullup(uint32_t base, uint8_t pin);
static void gpio_output_od_high(uint32_t base, uint8_t pin);
static void KeyboardMatrix_GPIO_Init(void);
static uint8_t KeyboardMatrix_ScanRow(uint8_t row);
static void KeyboardMatrix_ScanRows(uint8_t rows[8]);

typedef struct
{
  uint8_t valid;
  uint8_t key_index;
  uint8_t contact_mask;
} KeyEvent;

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

  /* Aviso corto de arranque. No codifica teclas. */
  HAL_Delay(500);
  beep(880, 80);
  pause_ms(120);

  /* Si alguna tecla quedo tomada al arrancar, esperar liberacion. */
  if (any_key_now())
  {
    beep(180, 120);
    wait_release();
  }

  while (1)
  {
    KeyEvent ev = KeyboardMatrix_ScanStable();

    if (ev.valid)
    {
      play_note_while_held(ev.key_index);
      pause_ms(8);
    }
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
  if (half_period_us < 20U)
  {
    half_period_us = 20U;
  }

  uint32_t cycles = (duration_ms * freq_hz) / 1000UL;
  if (cycles < 1U)
  {
    cycles = 1U;
  }

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
  if (count == 0U)
  {
    beep(160, 120);
    pause_ms(150);
    return;
  }

  for (uint8_t i = 0; i < count; i++)
  {
    beep(freq_hz, 55);
    pause_ms(55);
  }
  pause_ms(220);
}

static void report_start(void)
{
  beep(440, 90); pause_ms(80);
  beep(660, 90); pause_ms(80);
  beep(880, 150); pause_ms(300);
}

static void report_ready(void)
{
  beep(880, 90); pause_ms(60);
  beep(1100, 90); pause_ms(60);
  beep(1320, 150); pause_ms(250);
}

static uint32_t note_freq_for_key(uint8_t key_index)
{
  /*
   * Mapa directo basado en la fórmula observada en el firmware oficial:
   *   nota aproximada = 72 - keyIndex
   *
   * key 0  = C5  aprox 523 Hz
   * key 31 = F2  aprox  87 Hz
   *
   * Si queda invertido físicamente, basta invertir esta tabla en V24.
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

static void play_note_while_held(uint8_t key_index)
{
  uint32_t freq_hz = note_freq_for_key(key_index);
  uint32_t half_period_us = 500000UL / freq_hz;
  uint8_t phase = 0U;
  uint8_t tick = 0U;

  if (half_period_us < 20U)
  {
    half_period_us = 20U;
  }

  while (1)
  {
    if (phase == 0U)
    {
      Bare_DAC_Set(DAC_HIGH, DAC_MID);
      phase = 1U;
    }
    else
    {
      Bare_DAC_Set(DAC_LOW, DAC_MID);
      phase = 0U;
    }

    delay_us(half_period_us);

    /* Re-escanear varias veces por ciclo para mantener la nota mientras la tecla siga presionada. */
    tick++;
    if (tick >= 8U)
    {
      uint8_t rows[8];
      KeyboardMatrix_ScanRows(rows);
      KeyEvent now = KeyboardMatrix_FindFirstKey(rows);

      if (!now.valid || now.key_index != key_index)
      {
        break;
      }

      tick = 0U;
    }
  }

  Bare_DAC_Set(DAC_MID, DAC_MID);
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
  delay_us(20);

  /* Activar una seleccion en LOW. */
  GPIO_ODR(GPIOE_BASE_ADDR) &= ~(1UL << (8U + row));
  delay_us(80);

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
