// ======================================================================
// \title  STM32UartDriver.cpp
// \brief  cpp file for the STM32H7 USART1 DMA-backed byte stream driver
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32UartDriver/Stm32UartDriver.hpp>

#include "CacheMaintenance.hpp"
#include "dma.h"
#include "usart.h"

namespace {

volatile bool s_txDmaBusy = false;
volatile bool s_rxChunkReady = false;
volatile uint16_t s_rxChunkLen = 0;
volatile bool s_uartErrorPending = false;
volatile uint32_t s_uartErrorCode = 0;

}  // namespace

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    FW_ASSERT(huart != nullptr);
    if (huart->Instance == USART1) {
        s_txDmaBusy = false;
    }
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size) {
    FW_ASSERT(huart != nullptr);
    if (huart->Instance == USART1) {
        s_rxChunkLen = Size;
        s_rxChunkReady = true;
    }
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    FW_ASSERT(huart != nullptr);
    if (huart->Instance == USART1) {
        s_uartErrorCode = huart->ErrorCode;
        s_uartErrorPending = true;
    }
}

namespace Drv {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

Stm32UartDriver ::Stm32UartDriver(const char* const compName)
    : Stm32UartDriverComponentBase(compName),
      m_txHead(0),
      m_txCount(0),
      m_txTimeoutUs(0),
      m_baudRate(0),
      m_rxHead(0),
      m_rxCount(0),
      m_allocationSize(0),
      m_bytesSent(0),
      m_bytesReceived(0),
      m_txErrorCount(0),
      m_rxErrorCount(0) {}

Stm32UartDriver ::~Stm32UartDriver() {}

Fw::Success Stm32UartDriver :: open(FwSizeType allocationSize, IRQn_Type IRQn, U32 preemptPriority, U32 subPriority, U32 baudRate) {
    this->m_allocationSize = allocationSize;

    // DMA1 clock/NVIC must be enabled before HAL_UART_MspInit() (invoked from
    // MX_USART1_UART_Init() -> HAL_UART_Init()) links and initializes the
    // USART1 TX/RX DMA streams.
    MX_DMA_Init();
    MX_USART1_UART_Init();

    this->m_baudRate = baudRate;
    FW_ASSERT(this->m_baudRate > 0);

    // Not configured by CubeMX: the USART1 global interrupt is required for
    // HAL_UARTEx_ReceiveToIdle_DMA()'s idle-line detection, which only the
    // USART peripheral (not the DMA streams) can signal.
    HAL_NVIC_SetPriority(IRQn, preemptPriority, subPriority);
    HAL_NVIC_EnableIRQ(IRQn);

    s_txDmaBusy = false;
    s_rxChunkReady = false;
    s_rxChunkLen = 0;
    s_uartErrorPending = false;
    s_uartErrorCode = 0;

    const HAL_StatusTypeDef status =
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, this->m_rxStaging, static_cast<uint16_t>(RX_STAGING_SIZE));
    if (status != HAL_OK) {
        Fw::LogStringArg _op("ReceiveToIdle_DMA");
        this->log_WARNING_HI_HalError(_op, static_cast<I32>(status));
        return Fw::Success::FAILURE;
    }

    this->log_ACTIVITY_HI_PortOpened(huart1.Init.BaudRate);
    if (this->isConnected_ready_OutputPort(0)) {
        this->ready_out(0);
    }
    return Fw::Success::SUCCESS;
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------

void Stm32UartDriver ::poll() {
    if (s_uartErrorPending) {
        this->recoverUartError();
    }

    this->pollRx();
    this->pollTx();
}

void Stm32UartDriver ::run_handler(FwIndexType portNum, U32 context) {
    this->tlmWrite_BytesSent(this->m_bytesSent);
    this->tlmWrite_BytesRecv(this->m_bytesReceived);
}

Drv::ByteStreamStatus Stm32UartDriver ::send_handler(FwIndexType portNum, Fw::Buffer& serBuffer) {
    if (!serBuffer.isValid()) {
        return Drv::ByteStreamStatus::OTHER_ERROR;
    }

    const FwSizeType size = serBuffer.getSize();
    const FwSizeType free = TX_RING_SIZE - this->m_txCount;
    if (size > free) {
        this->log_WARNING_HI_TxRingFull(static_cast<U32>(size), static_cast<U32>(free));
        return Drv::ByteStreamStatus::SEND_RETRY;
    }

    const U8* data = serBuffer.getData();
    for (FwSizeType i = 0; i < size; i++) {
        this->m_txRing[(this->m_txHead + i) % TX_RING_SIZE] = data[i];
    }
    this->m_txHead = (this->m_txHead + size) % TX_RING_SIZE;
    this->m_txCount += size;

    return Drv::ByteStreamStatus::OP_OK;
}

void Stm32UartDriver ::recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    this->deallocate_out(0, fwBuffer);
}

// ----------------------------------------------------------------------
// TX path
// ----------------------------------------------------------------------

void Stm32UartDriver ::pollTx() {
    if (s_txDmaBusy) {
        Os::RawTime now;
        (void)now.now();
        U32 elapsedUs = 0;
        (void)now.getDiffUsec(this->m_txDmaStart, elapsedUs);
        if (elapsedUs >= this->m_txTimeoutUs) {
            (void)HAL_UART_AbortTransmit(&huart1);
            s_txDmaBusy = false;
            this->m_txErrorCount++;
            this->log_WARNING_HI_TxTimeout(elapsedUs);
        }
        return;
    }

    if (this->m_txCount == 0) {
        return;
    }

    const FwSizeType toSend = (this->m_txCount < TX_STAGING_SIZE) ? this->m_txCount : TX_STAGING_SIZE;
    const FwSizeType tail = (this->m_txHead + TX_RING_SIZE - this->m_txCount) % TX_RING_SIZE;
    for (FwSizeType i = 0; i < toSend; i++) {
        this->m_txStaging[i] = this->m_txRing[(tail + i) % TX_RING_SIZE];
    }
    this->m_txCount -= toSend;

    Stm32::CleanDCacheForDma(this->m_txStaging, toSend);

    const U32 onWireUs =
        static_cast<U32>((static_cast<U64>(toSend) * TX_BITS_PER_BYTE * 1000000U) / this->m_baudRate);
    this->m_txTimeoutUs = (onWireUs * TX_TIMEOUT_MARGIN) + TX_TIMEOUT_SLACK_US;

    (void)this->m_txDmaStart.now();
    s_txDmaBusy = true;
    const HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart1, this->m_txStaging, static_cast<uint16_t>(toSend));
    if (status != HAL_OK) {
        s_txDmaBusy = false;
        this->m_txErrorCount++;
        Fw::LogStringArg _op("Transmit_DMA");
        this->log_WARNING_HI_HalError(_op, static_cast<I32>(status));
    } else {
        this->m_bytesSent += toSend;
    }
}

// ----------------------------------------------------------------------
// RX path
// ----------------------------------------------------------------------

void Stm32UartDriver ::pollRx() {
    if (s_rxChunkReady) {
        const FwSizeType len = s_rxChunkLen;
        s_rxChunkReady = false;
        s_rxChunkLen = 0;

        Stm32::InvalidateDCacheForDma(this->m_rxStaging, RX_STAGING_SIZE);

        const FwSizeType freeSpace = RX_RING_SIZE - this->m_rxCount;
        const FwSizeType toCopy = (len < freeSpace) ? len : freeSpace;
        for (FwSizeType i = 0; i < toCopy; i++) {
            this->m_rxRing[(this->m_rxHead + i) % RX_RING_SIZE] = this->m_rxStaging[i];
        }
        this->m_rxHead = (this->m_rxHead + toCopy) % RX_RING_SIZE;
        this->m_rxCount += toCopy;

        if (toCopy < len) {
            this->m_rxErrorCount++;
            this->log_WARNING_HI_RxRingFull(static_cast<U32>(len - toCopy));
        }

        // Re-arm immediately so no bytes are lost while the ring/Fw::Buffer
        // drain below runs.
        (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart1, this->m_rxStaging, static_cast<uint16_t>(RX_STAGING_SIZE));
    }

    if (this->m_rxCount == 0) {
        return;
    }

    Fw::Buffer buff = this->allocate_out(0, this->m_allocationSize);
    if (buff.getData() == nullptr) {
        this->log_WARNING_HI_NoBuffers();
        return;  // leave data queued in the ring; retry next poll
    }

    const FwSizeType tail = (this->m_rxHead + RX_RING_SIZE - this->m_rxCount) % RX_RING_SIZE;
    const FwSizeType copySize = (this->m_rxCount < buff.getSize()) ? this->m_rxCount : buff.getSize();
    for (FwSizeType i = 0; i < copySize; i++) {
        buff.getData()[i] = this->m_rxRing[(tail + i) % RX_RING_SIZE];
    }
    this->m_rxCount -= copySize;

    buff.setSize(copySize);
    this->m_bytesReceived += copySize;
    this->recv_out(0, buff, Drv::ByteStreamStatus::OP_OK);
}

void Stm32UartDriver ::recoverUartError() {
    const U32 errorCode = s_uartErrorCode;
    s_uartErrorPending = false;
    s_uartErrorCode = 0;

    (void)HAL_UART_AbortTransmit(&huart1);
    (void)HAL_UART_AbortReceive(&huart1);
    s_txDmaBusy = false;

    if ((errorCode & (HAL_UART_ERROR_ORE | HAL_UART_ERROR_FE | HAL_UART_ERROR_PE | HAL_UART_ERROR_NE)) != 0U) {
        this->m_rxErrorCount++;
    }
    if ((errorCode & HAL_UART_ERROR_DMA) != 0U) {
        this->m_txErrorCount++;
        this->m_rxErrorCount++;
    }
    this->log_WARNING_HI_UartError(errorCode);

    huart1.ErrorCode = HAL_UART_ERROR_NONE;

    s_rxChunkReady = false;
    s_rxChunkLen = 0;
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart1, this->m_rxStaging, static_cast<uint16_t>(RX_STAGING_SIZE));
}

}  // namespace Drv