# NUCLEO-F429ZI boilerplate (CMake + HAL)

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
    cmake -DCMAKE_BUILD_TYPE=Debug -B build
    cmake --build build             # gera build/nucleo_f429zi.elf e .bin
    cmake --build build -t flash    # via OpenOCD + ST-LINK

## Estrutura
    Core/Src/main.cpp        clock, GPIO, UARTs, helper esp_at()
    Core/Src/syscalls.c      printf -> USART3
    Core/Inc/stm32f4xx_hal_conf.h   so RCC/GPIO/DMA/UART/PWR/FLASH/CORTEX ligados
    STM32F429ZITx_FLASH.ld   linker script (2 MB flash, 192 KB RAM)

## C++ layers
    Core/Inc/rxtx.hpp   RxTx<Derived>: CRTP transport interface (write/read/flush/millis), no vtable
    Core/Inc/uart.hpp   Uart: HAL polling implementation of RxTx
    Core/Inc/wifi.hpp   WiFi<Port>: ESP-AT client (begin, connect, httpGet/httpPost/httpRequest)
    tests/host_test.cpp host test with a fake port:
      g++ -std=c++17 -Wall -Wextra -I Core/Inc tests/host_test.cpp -o /tmp/host_test && /tmp/host_test
Set WIFI_SSID / WIFI_PASS in Core/Src/main.cpp. HTTP needs "AT http command support" in the ESP-AT build.
