#!/usr/bin/env bash
# Baixa os drivers da ST (HAL + CMSIS) como submodulos git.
set -e
[ -d .git ] || git init -q
add() { [ -d "$2" ] || git submodule add --depth 1 "$1" "$2"; }
add https://github.com/STMicroelectronics/stm32f4xx_hal_driver.git drivers/stm32f4xx_hal_driver
add https://github.com/STMicroelectronics/cmsis_device_f4.git      drivers/cmsis_device_f4
add https://github.com/STMicroelectronics/cmsis_core.git           drivers/cmsis_core
