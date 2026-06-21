/* USER CODE BEGIN Header */
/*
 * KeyStep Custom DC V4
 *
 * Prueba de diagnóstico, NO audio:
 * - Elimina TIM6/interrupciones.
 * - Escribe directo a registros del STM32F103VCT6.
 * - Alterna DAC1/PA4 y DAC2/PA5 cada 500 ms.
 *
 * Objetivo:
 * - Confirmar si realmente salen voltajes por los DAC.
 * - Medir con multímetro o entrada CV.
 *
 * DAC1 / PA4 debería alternar bajo/alto.
 * DAC2 / PA5 debería alternar al revés.
 *
 * No implementa USB MIDI.
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
#define DAC_SWTRIGR_REG   (*(volatile uint32_t*)0x40007404UL)

/* RCC bits */
#define RCC_APB2ENR_IOPAEN   (1UL << 2)
#define RCC_APB1ENR_DACEN    (1UL << 29)

/* DAC CR bits */
#define DAC_CR_EN1           (1UL << 0)
#define DAC_CR_BOFF1         (1UL << 1)
#define DAC_CR_EN2           (1UL << 16)
#define DAC_CR_BOFF2         (1UL << 17)

/* Valores 12-bit */
#define DAC_LOW              128U
#define DAC_MID              2048U
#define DAC_HIGH             3968U

/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */
static void Bare_DAC_Init(void);
static void Bare_DAC_Set(uint16_t ch1, uint16_t ch2);
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
  Bare_DAC_Init();

  /* Estado inicial visible con multímetro */
  Bare_DAC_Set(DAC_MID, DAC_MID);
  HAL_Delay(1000);
  /* USER CODE END 2 */

  while (1)
  {
    /*
     * Alternancia lenta:
     * DAC1 alto / DAC2 bajo
     */
    Bare_DAC_Set(DAC_HIGH, DAC_LOW);
    HAL_Delay(500);

    /*
     * DAC1 bajo / DAC2 alto
     */
    Bare_DAC_Set(DAC_LOW, DAC_HIGH);
    HAL_Delay(500);
  }
}

/* USER CODE BEGIN 4 */

static void Bare_DAC_Init(void)
{
  /*
   * Habilitar GPIOA y DAC.
   * En STM32F103, DAC está en APB1 bit 29.
   */
  RCC_APB2ENR_REG |= RCC_APB2ENR_IOPAEN;
  RCC_APB1ENR_REG |= RCC_APB1ENR_DACEN;

  /*
   * PA4 y PA5 en modo analog:
   * GPIOA_CRL configura PA0..PA7, 4 bits por pin.
   * PA4 bits 19:16, PA5 bits 23:20.
   * Analog input mode = 0000.
   */
  GPIOA_CRL_REG &= ~((0xFUL << 16) | (0xFUL << 20));

  /*
   * Habilitar DAC canal 1 y canal 2.
   * Dejamos output buffer ON, por eso NO seteamos BOFF.
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
