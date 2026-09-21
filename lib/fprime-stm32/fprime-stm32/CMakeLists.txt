set(FPRIME_STM32_HAL_SOURCES
    Hal/stm32h753_hal/Core/Src/system_stm32h7xx.c
    Hal/stm32h753_hal/Core/Src/stm32h7xx_hal_msp.c
    # NOTE: Hal/stm32h753_hal/Core/Src/stm32h7xx_it.c is intentionally
    # NOT compiled into this static library.
    Hal/config/src/stm32h7_clock.c
    Hal/stm32h753_hal/Core/Src/dma.c
    Hal/stm32h753_hal/Core/Src/gpio.c
    Hal/stm32h753_hal/Core/Src/i2c.c
    Hal/stm32h753_hal/Core/Src/tim.c
    Hal/config/src/tim2_clock.cpp
    Hal/stm32h753_hal/Core/Src/usart.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
    Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
)

add_library(FprimeStm32Config INTERFACE)
target_include_directories(FprimeStm32Config INTERFACE "${CMAKE_CURRENT_LIST_DIR}/Drv/config")


if (FPRIME_PLATFORM STREQUAL "stm32h7")
    add_library(FprimeStm32 STATIC ${FPRIME_STM32_HAL_SOURCES})

    target_include_directories(FprimeStm32 PUBLIC
        "${PROJECT_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}"
        "${FPRIME_PROJECT_ROOT}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/Drv/STM32UartDriver"
        "${CMAKE_CURRENT_BINARY_DIR}/Drv/STM32Timer"
        "${CMAKE_CURRENT_LIST_DIR}/Drv/config"
        "${CMAKE_CURRENT_LIST_DIR}/Hal/config/include"
        "${CMAKE_CURRENT_LIST_DIR}/Hal/stm32h753_hal/Core/Inc"
        "${CMAKE_CURRENT_LIST_DIR}/Hal/stm32h753_hal/Drivers/CMSIS/Include"
        "${CMAKE_CURRENT_LIST_DIR}/Hal/stm32h753_hal/Drivers/CMSIS/Device/ST/STM32H7xx/Include"
        "${CMAKE_CURRENT_LIST_DIR}/Hal/stm32h753_hal/Drivers/STM32H7xx_HAL_Driver/Inc"
    )

    target_compile_definitions(FprimeStm32 PUBLIC STM32H753xx USE_HAL_DRIVER USE_PWR_LDO_SUPPLY)

    target_compile_options(FprimeStm32 PRIVATE -ffunction-sections -fdata-sections)

    target_link_libraries(FprimeStm32 PUBLIC PlatformStm32h7)
    add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Os")
endif()

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Drv")