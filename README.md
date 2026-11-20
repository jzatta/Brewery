# NUCLEO-F429ZI (CMake + HAL)

- SYSCLK 180 MHz (HSE 8 MHz do ST-LINK)
- LD1 (PB0) pisca; LD3 (PB14) acende com o botao B1
- `printf()` na USART3 -> ST-LINK VCP (`/dev/ttyACM*`, 115200)
- USART6 (PG14 TX = D1, PG9 RX = D0) para o modulo ESP-AT, 115200

## Ligacao com o ESP32-C5
| Nucleo        | ESP32-C5 (pinos do AT) |
|---------------|------------------------|
| PG14 (TX, D1) | RX                     |
| PG9  (RX, D0) | TX                     |
| GND           | GND                    |

Nivel logico 3,3 V nos dois lados.

## Build
    cmake -DCMAKE_BUILD_TYPE=Debug -Bbuild
    cmake --build build
    cmake --build build --target flash

Com Docker:
    docker build -t stm32-build .
    docker run -it --rm -v $PWD:/work stm32-build bash -c \
      "cmake -G Ninja -B build && ninja -C build"
Para gravar pelo container, adicione `--privileged -v /dev/bus/usb:/dev/bus/usb`
ou grave no host com `st-flash`/OpenOCD.

## Estrutura
    Core/Src/main.c          clock, GPIO, UARTs, helper esp_at()
    Core/Src/syscalls.c      printf -> USART3
    Core/Inc/stm32f4xx_hal_conf.h   so RCC/GPIO/DMA/UART/PWR/FLASH/CORTEX ligados
    STM32F429ZITx_FLASH.ld   linker script (2 MB flash, 192 KB RAM)
