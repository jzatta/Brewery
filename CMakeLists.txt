cmake_minimum_required(VERSION 3.22)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi.cmake)
project(nucleo_f429zi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(HAL  drivers/stm32f4xx_hal_driver)
set(CMSD drivers/cmsis_device_f4)
set(CMSC drivers/cmsis_core/CMSIS/Core)

set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

file(GLOB HAL_SRC ${HAL}/Src/stm32f4xx_hal*.c)
list(FILTER HAL_SRC EXCLUDE REGEX "_template\\.c$")

add_executable(${PROJECT_NAME}.elf
    Core/Src/main.cpp
    Core/Src/stm32f4xx_it.c
    Core/Src/syscalls.c
    Core/Src/system_stm32f4xx.c
    Core/Startup/startup_stm32f429xx.s
    ${HAL_SRC})

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F429xx USE_HAL_DRIVER)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    Core/Inc
    ${HAL}/Inc
    ${CMSD}/Include
    ${CMSC}/Include)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS} -Wall -Wextra -ffunction-sections -fdata-sections
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit>
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/STM32F429ZITx_FLASH.ld
    -Wl,--gc-sections -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--no-warn-rwx-segments
    --specs=nano.specs --specs=nosys.specs -u _printf_float)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>)

add_custom_target(flash
    COMMAND openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
            -c "program ${PROJECT_NAME}.elf verify reset exit"
    DEPENDS ${PROJECT_NAME}.elf)
