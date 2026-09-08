set(FPRIME_STM32_HAL_SOURCES
    src/system_stm32h7xx.c
    src/stm32h7xx_hal_msp.c
    # NOTE: src/stm32h7xx_it.c is intentionally NOT compiled into this static
    # library. GNU ld only extracts an .o from a static archive when something
    # already linked has an *undefined* reference into it -- but
    # startup/startup_stm32h753xx.s already provides `.weak` Default_Handler
    # aliases for every vector table entry (and is linked directly into the
    # executable, not via an archive), so every vector is "satisfied" before
    # the linker ever needs to search this archive. That silently discards the
    # real ISR bodies (SysTick_Handler/HAL_IncTick, HardFault_Handler,
    # DMA1_Stream0/1_IRQHandler, TIM2_IRQHandler, ...) in favor of the weak
    # infinite-loop aliases. It is instead compiled directly into the
    # ReferenceDeployment executable's own sources -- see
    # FprimeBaremetalReference/Deployments/ReferenceDeployment/CMakeLists.txt
    # -- so its strong definitions are
    # always present and correctly override the weak ones.
    src/stm32h7_clock.c
    src/dma.c
    src/gpio.c
    src/tim.c
    src/tim2_clock.cpp
    src/usart.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
)

add_library(FprimeStm32 STATIC ${FPRIME_STM32_HAL_SOURCES})

target_include_directories(FprimeStm32 PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}"
    "${FPRIME_PROJECT_ROOT}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/Components/Stm32Timer"
    "${CMAKE_CURRENT_BINARY_DIR}/Drv/STM32UartDriver"
    "${CMAKE_CURRENT_LIST_DIR}/config"
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/Drivers/CMSIS/Include"
    "${CMAKE_CURRENT_LIST_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Include"
    "${CMAKE_CURRENT_LIST_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc"
)

target_compile_definitions(FprimeStm32 PUBLIC STM32H753xx USE_HAL_DRIVER)

target_compile_options(FprimeStm32 PRIVATE -ffunction-sections -fdata-sections)

target_link_libraries(FprimeStm32 PUBLIC PlatformStm32h7)
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Components")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Os")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Drv")