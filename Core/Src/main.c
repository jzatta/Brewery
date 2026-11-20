/*
 * NUCLEO-F429ZI boilerplate
 *  - SYSCLK 180 MHz (HSE 8 MHz do ST-LINK, bypass, overdrive)
 *  - LEDs: LD1 verde PB0, LD2 azul PB7, LD3 vermelho PB14
 *  - Botao B1: PC13
 *  - USART3 (PD8 TX / PD9 RX) -> ST-LINK VCP, usada pelo printf()
 *  - USART6 (PG14 TX / PG9 RX = Arduino D1 / D0) -> modulo ESP-AT, 115200 8N1
 */
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"

UART_HandleTypeDef huart3;  /* debug / printf */
UART_HandleTypeDef huart6;  /* ESP-AT         */

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART_Init(void);

/* Envia um comando AT e imprime a resposta (polling, simples). */
static void esp_at(const char *cmd, uint32_t timeout_ms)
{
    uint8_t c;
    uint32_t t0;

    printf(">> %s\r\n", cmd);
    HAL_UART_Transmit(&huart6, (uint8_t *)cmd, strlen(cmd), 100);
    HAL_UART_Transmit(&huart6, (uint8_t *)"\r\n", 2, 100);

    t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < timeout_ms) {
        if (HAL_UART_Receive(&huart6, &c, 1, 10) == HAL_OK) {
            putchar(c);
            t0 = HAL_GetTick(); /* renova o timeout enquanto chegam dados */
        }
    }
    printf("\r\n");
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART_Init();

    printf("\r\nNUCLEO-F429ZI @ %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000UL);

    esp_at("AT", 500);
    esp_at("AT+GMR", 500);

    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
        }
        HAL_Delay(500);
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_BYPASS;           /* MCO do ST-LINK, 8 MHz */
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 4;                        /* 8/4   = 2 MHz   */
    osc.PLL.PLLN       = 180;                      /* 2*180 = 360 MHz */
    osc.PLL.PLLP       = RCC_PLLP_DIV2;            /* 180 MHz         */
    osc.PLL.PLLQ       = 8;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) { while (1) {} }

    if (HAL_PWREx_EnableOverDrive() != HAL_OK) { while (1) {} }

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;          /* 180 MHz */
    clk.APB1CLKDivider = RCC_HCLK_DIV4;            /*  45 MHz */
    clk.APB2CLKDivider = RCC_HCLK_DIV2;            /*  90 MHz */
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) { while (1) {} }
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    g.Pin   = GPIO_PIN_0 | GPIO_PIN_7 | GPIO_PIN_14;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    g.Pin  = GPIO_PIN_13;                          /* B1 (ativo em nivel alto) */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOC, &g);
}

static void UART_Init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();

    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;

    g.Pin       = GPIO_PIN_8 | GPIO_PIN_9;         /* USART3 TX/RX */
    g.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &g);

    g.Pin       = GPIO_PIN_14 | GPIO_PIN_9;        /* USART6 TX/RX */
    g.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOG, &g);

    huart3.Instance          = USART3;
    huart3.Init.BaudRate     = 115200;
    huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    huart3.Init.StopBits     = UART_STOPBITS_1;
    huart3.Init.Parity       = UART_PARITY_NONE;
    huart3.Init.Mode         = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart3);

    huart6 = huart3;
    huart6.Instance = USART6;
    HAL_UART_Init(&huart6);
}
