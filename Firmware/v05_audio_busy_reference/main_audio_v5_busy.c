/* USER CODE BEGIN Header */
/*
 * KeyStep Custom Audio V5 Busy
 *
 * Diagnóstico audible sin TIM6 ni interrupciones:
 * - Ya confirmamos que DC V4 alterna voltaje.
 * - Este firmware usa los mismos registros directos del DAC.
 * - Genera "BEEP BEEP" en DAC1/PA4/Pitch CV usando busy-wait.
 * - DAC2/PA5/Mod CV alterna lento como marcador.
 *
 * No implementa USB MIDI.
 *
 * Prueba recomendada:
 * - Salida CV -> entrada CV/audio con atenuación/mixer.
 * - No usar audífonos directo al CV.
 */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN PV */

#define APP_BASE_ADDRESS 0x08004000UL

/* Registros STM32F103 */
#define RCC_APB2ENR_REG   (*(volatile uint32_t*)0x40021018UL)
#define RCC_APB1ENR_REG   (*(volatile uint32_t*)0x4002101CUL)
#define GPIOA_CRL_REG     (*(volatile uint32_t*)0x40010800UL)

#define DAC_CR_REG        (*(volatile uint32_t*)0x40007400UL)
#define DAC_DHR12R1_REG   (*(volatile uint32_t*)0x40007408UL)
#define DAC_DHR12R2_REG   (*(volatile uint32_t*)0x40007414UL)

/* RCC bits */
#define RCC_APB2ENR_IOPAEN   (1UL << 2)
#define RCC_APB1ENR_DACEN    (1UL << 29)

/* DAC CR bits */
#define DAC_CR_EN1           (1UL << 0)
#define DAC_CR_EN2           (1UL << 16)

/* DAC values */
#define DAC_LOW              180U
#define DAC_MID              2048U
#define DAC_HIGH             3920U

/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */
static void Bare_DAC_Init(void);
static void Bare_DAC_Set(uint16_t ch1, uint16_t ch2);
static void DWT_Init_Counter(void);
static void delay_us(uint32_t us);
static void beep_dac1(uint32_t freq_hz, uint32_t duration_ms, uint16_t dac2_marker);
/* USER CODE END PFP */

int main(void)
{
  /* USER CODE BEGIN 1 */
  SCB->VTOR = APP_BASE_ADDRESS;
  __DSB();
  __ISB();
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  DWT_Init_Counter();
  Bare_DAC_Init();

  Bare_DAC_Set(DAC_MID, DAC_MID);
  HAL_Delay(500);
  /* USER CODE END 2 */

  while (1)
  {
    /*
     * Patrón audible:
     * - beep grave 220 Hz
     * - pausa
     * - beep medio 440 Hz
     * - pausa
     * - beep alto 880 Hz
     * - pausa larga
     *
     * DAC2 queda como marcador DC alternado.
     */
    beep_dac1(220, 180, DAC_LOW);
    HAL_Delay(120);

    beep_dac1(440, 180, DAC_HIGH);
    HAL_Delay(120);

    beep_dac1(880, 180, DAC_LOW);
    HAL_Delay(500);
  }
}

/* USER CODE BEGIN 4 */

static void Bare_DAC_Init(void)
{
  RCC_APB2ENR_REG |= RCC_APB2ENR_IOPAEN;
  RCC_APB1ENR_REG |= RCC_APB1ENR_DACEN;

  /*
   * PA4 y PA5 en modo analog.
   */
  GPIOA_CRL_REG &= ~((0xFUL << 16) | (0xFUL << 20));

  /*
   * DAC canal 1 y 2 ON, buffer ON.
   */
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

static void DWT_Init_Counter(void)
{
  /*
   * Habilita contador de ciclos DWT en Cortex-M3.
   */
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

static void beep_dac1(uint32_t freq_hz, uint32_t duration_ms, uint16_t dac2_marker)
{
  if (freq_hz == 0)
  {
    return;
  }

  uint32_t half_period_us = 500000UL / freq_hz;
  uint32_t cycles = (duration_ms * freq_hz) / 1000UL;

  if (cycles < 1)
  {
    cycles = 1;
  }

  for (uint32_t i = 0; i < cycles; i++)
  {
    Bare_DAC_Set(DAC_HIGH, dac2_marker);
    delay_us(half_period_us);

    Bare_DAC_Set(DAC_LOW, dac2_marker);
    delay_us(half_period_us);
  }

  Bare_DAC_Set(DAC_MID, dac2_marker);
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
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
