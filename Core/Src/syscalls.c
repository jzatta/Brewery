/* Redireciona printf() para a USART3 (ST-LINK Virtual COM Port). */
#include <errno.h>
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart3;

int _write(int fd, char *buf, int len)
{
    (void)fd;
    return (HAL_UART_Transmit(&huart3, (uint8_t *)buf, len, HAL_MAX_DELAY) == HAL_OK) ? len : -1;
}
