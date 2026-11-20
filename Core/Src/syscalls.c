/* Redireciona printf() para a USART3 (ST-LINK Virtual COM Port). */
#include <errno.h>
#include <sys/stat.h>
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart3;

int _write(int fd, char *buf, int len)
{
    (void)fd;
    return (HAL_UART_Transmit(&huart3, (uint8_t *)buf, len, HAL_MAX_DELAY) == HAL_OK) ? len : -1;
}

int _close(int fd)  { (void)fd; return -1; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int off, int whence) { (void)fd; (void)off; (void)whence; return 0; }
int _read(int fd, char *buf, int len)   { (void)fd; (void)buf; (void)len; return 0; }
int _getpid(void)   { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;   /* diz à libc que o stdout é um dispositivo de caracteres */
    return 0;
}