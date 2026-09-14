// ======================================================================
// \title  Stm32UartDriver.cpp
// \author ivanlara
// \brief  HAL boundary for the STM32H7 USART1 DMA-backed byte stream
//         driver (real hardware implementation, stm32h7 target only).
//         This is the only file in this driver allowed to include
//         CacheMaintenance.hpp/dma.h/usart.h -- see
//         Stm32UartDriverStub.cpp for the host unit-test stand-in.
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32UartDriver/Stm32UartDriver.hpp>

#include "CacheMaintenance.hpp"
#include "dma.h"
#include "usart.h"

namespace {

//! Single-instance callback trampoline: the HAL callbacks below are free
//! functions with no user-context pointer, and the topology only ever
//! instantiates one Stm32UartDriver against the one physical USART1
//! peripheral.
Stm32::Stm32UartDriver* s_instance = nullptr;

}  // namespace

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    FW_ASSERT(huart != nullptr);
    if (huart->Instance == USART1 && s_instance != nullptr) {
        s_instance->signalTxComplete();
    }
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size) {
    FW_ASSERT(huart != nullptr);
    if (huart->Instance == USART1 && s_instance != nullptr) {
        s_instance->signalRxChunk(Size);
    }
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    FW_ASSERT(huart != nullptr);
    if (huart->Instance == USART1 && s_instance != nullptr) {
        s_instance->signalUartError(huart->ErrorCode);
    }
}

namespace Stm32 {

bool Stm32UartDriver ::hwOpen(I32 IRQn, U32 preemptPriority, U32 subPriority, U32 requestedBaudRate,
                               U32& outActualBaudRate) {
    (void)requestedBaudRate;  // not used to configure the peripheral: CubeMX fixes the baud in usart.c

    // DMA1 clock/NVIC must be enabled before HAL_UART_MspInit() (invoked from
    // MX_USART1_UART_Init() -> HAL_UART_Init()) links and initializes the
    // USART1 TX/RX DMA streams.
    MX_DMA_Init();
    MX_USART1_UART_Init();

    // Not configured by CubeMX: the USART1 global interrupt is required for
    // HAL_UARTEx_ReceiveToIdle_DMA()'s idle-line detection, which only the
    // USART peripheral (not the DMA streams) can signal.
    HAL_NVIC_SetPriority(static_cast<IRQn_Type>(IRQn), preemptPriority, subPriority);
    HAL_NVIC_EnableIRQ(static_cast<IRQn_Type>(IRQn));

    s_instance = this;
    this->m_txDmaBusy = false;
    this->m_rxChunkReady = false;
    this->m_rxChunkLen = 0;
    this->m_uartErrorPending = false;
    this->m_uartErrorCode = 0;

    const I32 status = this->hwRestartRx();
    if (status != HAL_OK) {
        Fw::LogStringArg _op("ReceiveToIdle_DMA");
        this->log_WARNING_HI_HalError(_op, status);
        return false;
    }

    outActualBaudRate = huart1.Init.BaudRate;
    return true;
}

bool Stm32UartDriver ::hwStartTx(const U8* data, FwSizeType len) {
    Stm32::CleanDCacheForDma(data, len);
    const HAL_StatusTypeDef status =
        HAL_UART_Transmit_DMA(&huart1, const_cast<U8*>(data), static_cast<uint16_t>(len));
    if (status != HAL_OK) {
        Fw::LogStringArg _op("Transmit_DMA");
        this->log_WARNING_HI_HalError(_op, static_cast<I32>(status));
        return false;
    }
    return true;
}

void Stm32UartDriver ::hwAbortTx() {
    (void)HAL_UART_AbortTransmit(&huart1);
}

void Stm32UartDriver ::hwInvalidateRxStaging() {
    Stm32::InvalidateDCacheForDma(this->m_rxStaging, RX_STAGING_SIZE);
}

I32 Stm32UartDriver ::hwRestartRx() {
    const HAL_StatusTypeDef status =
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, this->m_rxStaging, static_cast<uint16_t>(RX_STAGING_SIZE));
    return static_cast<I32>(status);
}

void Stm32UartDriver ::hwAbortRx() {
    (void)HAL_UART_AbortReceive(&huart1);
}

void Stm32UartDriver ::hwClearUartError() {
    huart1.ErrorCode = HAL_UART_ERROR_NONE;
}

void Stm32UartDriver ::hwClassifyUartError(U32 errorCode, bool& isRxAffecting, bool& isDmaAffecting) {
    isRxAffecting =
        (errorCode & (HAL_UART_ERROR_ORE | HAL_UART_ERROR_FE | HAL_UART_ERROR_PE | HAL_UART_ERROR_NE)) != 0U;
    isDmaAffecting = (errorCode & HAL_UART_ERROR_DMA) != 0U;
}

}  // namespace Stm32