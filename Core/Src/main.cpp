/*
 * NUCLEO-F429ZI boilerplate
 *  - SYSCLK 180 MHz (HSE 8 MHz do ST-LINK, bypass, overdrive)
 *  - LEDs: LD1 verde PB0, LD2 azul PB7, LD3 vermelho PB14
 *  - Botao B1: PC13
 *  - USART3 (PD8 TX / PD9 RX) -> ST-LINK VCP, usada pelo printf()
 *  - USART6 (PG14 TX / PG9 RX = Arduino D1 / D0) -> modulo ESP-AT, 115200 8N1
 */
#include <cstdio>
#include <cstring>
#include "stm32f4xx_hal.h"
#include "uart.hpp"
#include "wifi.hpp"

extern "C" {
UART_HandleTypeDef huart3;  /* debug / printf (usada por syscalls.c) */
}
UART_HandleTypeDef huart6;  /* ESP-AT         */

static Uart espUart(&huart6);
static WiFi wifi(espUart);          /* WiFi<Uart>, sem vtable */

static constexpr const char* WIFI_SSID = "YOUR_SSID";
static constexpr const char* WIFI_PASS = "YOUR_PASSWORD";

/* Exemplo de classe C++ com construtor executado antes do main() */
class Led {
public:
    Led(GPIO_TypeDef *port, uint16_t pin) : port_(port), pin_(pin) {}
    void toggle() { HAL_GPIO_TogglePin(port_, pin_); }
    void set(bool on) { HAL_GPIO_WritePin(port_, pin_, on ? GPIO_PIN_SET : GPIO_PIN_RESET); }
private:
    GPIO_TypeDef *port_;
    uint16_t pin_;
};

static Led ledGreen(GPIOB, GPIO_PIN_0);
static Led ledRed(GPIOB, GPIO_PIN_14);

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART_Init(void);

int main()
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART_Init();

    printf("\r\nNUCLEO-F429ZI (C++) @ %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000UL);

    if (!wifi.begin()) {
        printf("ESP-AT nao responde\r\n");
    } else if (!wifi.connect(WIFI_SSID, WIFI_PASS)) {
        printf("Falha no WiFi: %s\r\n", wifi.lastRaw());
    } else {
        HttpResponse r;
        if (wifi.httpPost("http://ntfy.sh/test222", "teste", r)) {
            printf("HTTP POST ok (%lu/%lu bytes):\r\n%s\r\n",
                   (unsigned long)r.length, (unsigned long)r.size, r.data);
        } else {
            printf("HTTP POST falhou: %s\r\n", wifi.lastRaw());
        }
    }

    while (1) {
        ledGreen.toggle();
        ledRed.set(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET);
        HAL_Delay(500);
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc{};
    RCC_ClkInitTypeDef clk{};

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
    GPIO_InitTypeDef g{};

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
    GPIO_InitTypeDef g{};

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
