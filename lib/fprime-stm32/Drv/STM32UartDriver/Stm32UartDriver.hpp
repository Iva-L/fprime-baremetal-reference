// ======================================================================
// \title  STM32UartDriver.hpp
// \brief  hpp file for the STM32H7 USART1 DMA-backed byte stream driver
//
// Non-blocking ground link driver for the bare-metal cyclic executive:
// send_handler()/recvReturnIn_handler() only touch fixed-size AXI SRAM ring
// buffers and return immediately; all DMA start/completion handling, error
// recovery, and Fw::Buffer ownership transfer happens from run_handler(),
// which is polled once per cyclic-executive pass via a rate group
// connection. HAL ISR callbacks (see STM32UartDriver.cpp) do nothing but
// latch volatile completion/error state.
// ======================================================================

#ifndef STM32_UART_DRIVER_HPP
#define STM32_UART_DRIVER_HPP

#include <lib/fprime-stm32/Drv/STM32UartDriver/Stm32UartDriverComponentAc.hpp>
#include <Fw/Types/SuccessEnumAc.hpp>
#include <Os/RawTime.hpp>
#include <UartDriverConfig.hpp>

namespace Drv {

class Stm32UartDriver final : public Stm32UartDriverComponentBase {
  public:
    //! Construct object Stm32UartDriver
    explicit Stm32UartDriver(const char* const compName);

    //! Configure USART1 (via the CubeMX-generated MX_DMA_Init()/
    //! MX_USART1_UART_Init(), fixed at the generated 115200 8N1 baud/frame
    //! settings in lib/fprime-stm32/src/usart.c) and arm the first RX
    //! reception. Must be called once from configureTopology().
    //! \return true on success
    //! \param allocationSize size of the AXI SRAM ring buffers
    //! \param preemptPriority NVIC preempt priority for the USART1 global interrupt
    //! \param subPriority NVIC subpriority for the USART1 global interrupt
    Fw::Success open(FwSizeType allocationSize, uint32_t preemptPriority, uint32_t subPriority);
    //! One bounded step of the DMA state machine: consume ISR-latched
    //! completion/error state, run cache maintenance, start the next transfer,
    //! and hand received data upstream. Called every cyclic-executive pass (not
    //! from a rate group) so DMA completions are serviced with minimal latency.
    void poll();

    //! Destroy object Stm32UartDriver
    ~Stm32UartDriver();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Telemetry emission only, connected to a rate group. The DMA state
    //! machine runs in poll(), which needs a far higher cadence than any rate
    //! group provides.
    void run_handler(FwIndexType portNum, U32 context) override;

    //! Synchronous send: enqueues into the TX ring buffer and returns.
    //! Rejects the whole request (no partial enqueue) if it does not fit,
    //! so the caller's buffer is either fully consumed or fully retained.
    Drv::ByteStreamStatus send_handler(FwIndexType portNum, Fw::Buffer& serBuffer) override;

    //! Port receiving back ownership of data sent out on $recv port
    void recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    // ----------------------------------------------------------------------
    // TX path: ring buffer -> DMA staging buffer -> USART1 DMA
    // ----------------------------------------------------------------------

    //! Start the next TX DMA transfer if the ring has data and DMA is idle;
    //! recover a stuck transfer once it exceeds its watchdog.
    void pollTx();

    static constexpr FwSizeType TX_RING_SIZE = Stm32UartDriverConfig::TX_RING_SIZE;
    static constexpr FwSizeType TX_STAGING_SIZE = Stm32UartDriverConfig::TX_STAGING_SIZE;
    static constexpr U32 TX_BITS_PER_BYTE = Stm32UartDriverConfig::TX_BITS_PER_BYTE;
    static constexpr U32 TX_TIMEOUT_MARGIN = Stm32UartDriverConfig::TX_TIMEOUT_MARGIN;
    static constexpr U32 TX_TIMEOUT_SLACK_US = Stm32UartDriverConfig::TX_TIMEOUT_SLACK_US;

    U8 m_txRing[TX_RING_SIZE];
    FwSizeType m_txHead;   //!< next free slot to write
    FwSizeType m_txCount;  //!< bytes currently buffered
    alignas(32) U8 m_txStaging[TX_STAGING_SIZE];
    Os::RawTime m_txDmaStart;
    U32 m_txTimeoutUs;  //!< watchdog for the in-flight transfer, sized from its length
    U32 m_baudRate;     //!< configured USART1 baud, cached for watchdog sizing

    // ----------------------------------------------------------------------
    // RX path: USART1 DMA -> DMA staging buffer -> ring buffer -> Fw::Buffer
    // ----------------------------------------------------------------------

    //! Drain a completed RX DMA staging chunk into the RX ring and re-arm
    //! reception, then forward whatever is queued in the ring upstream via
    //! allocate_out()/recv_out().
    void pollRx();

    //! Recover from a latched USART1/DMA error: abort in-flight transfers,
    //! clear HAL error state, report it, and re-arm reception.
    void recoverUartError();

    static constexpr FwSizeType RX_RING_SIZE = Stm32UartDriverConfig::RX_RING_SIZE;
    static constexpr FwSizeType RX_STAGING_SIZE = Stm32UartDriverConfig::RX_STAGING_SIZE;

    U8 m_rxRing[RX_RING_SIZE];
    FwSizeType m_rxHead;
    FwSizeType m_rxCount;
    alignas(32) U8 m_rxStaging[RX_STAGING_SIZE];

    FwSizeType m_allocationSize;  //!< size of each Fw::Buffer allocation request

    FwSizeType m_bytesSent;
    FwSizeType m_bytesReceived;
    U32 m_txErrorCount;
    U32 m_rxErrorCount;
};

}  // namespace Drv

#endif
