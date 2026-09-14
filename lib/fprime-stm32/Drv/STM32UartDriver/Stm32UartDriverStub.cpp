// ======================================================================
// \title  Stm32UartDriverStub.cpp
// \author ivanlara
// \brief  HAL boundary stand-in for host-native unit tests. No USART1/DMA
//         hardware exists on the build host, so every call here is a
//         fixed-behavior stub -- no HAL/CMSIS include, no register access.
//         A unit test drives the driver's state machine by calling
//         signalTxComplete()/signalRxChunk()/signalUartError() directly
//         (see Stm32UartDriver.hpp) instead of relying on a real ISR.
//         Swapped in for Stm32UartDriver.cpp by
//         Drv/STM32UartDriver/CMakeLists.txt's register_fprime_ut().
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32UartDriver/Stm32UartDriver.hpp>

namespace {

// Mirrors stm32h7xx_hal_uart.h's HAL_UART_ERROR_* bit assignments
// (PE=0x01, NE=0x02, FE=0x04, ORE=0x08, DMA=0x10) so a unit test injecting
// an error code via signalUartError() sees the same classification a real
// HAL error would produce.
constexpr U32 RX_AFFECTING_ERROR_MASK = 0x01U | 0x02U | 0x04U | 0x08U;
constexpr U32 DMA_AFFECTING_ERROR_MASK = 0x10U;

}  // namespace

namespace Stm32 {

bool Stm32UartDriver ::hwOpen(I32 IRQn, U32 preemptPriority, U32 subPriority, U32 requestedBaudRate,
                               U32& outActualBaudRate) {
    outActualBaudRate = requestedBaudRate;  // no peripheral to configure; echo back what was requested
    return true;
}

bool Stm32UartDriver ::hwStartTx(const U8* data, FwSizeType len) {
    return true;
}

void Stm32UartDriver ::hwAbortTx() {}

void Stm32UartDriver ::hwInvalidateRxStaging() {}

I32 Stm32UartDriver ::hwRestartRx() {
    return 0;  // 0 mirrors HAL_OK
}

void Stm32UartDriver ::hwAbortRx() {}

void Stm32UartDriver ::hwClearUartError() {}

void Stm32UartDriver ::hwClassifyUartError(U32 errorCode, bool& isRxAffecting, bool& isDmaAffecting) {
    isRxAffecting = (errorCode & RX_AFFECTING_ERROR_MASK) != 0U;
    isDmaAffecting = (errorCode & DMA_AFFECTING_ERROR_MASK) != 0U;
}

}  // namespace Stm32
