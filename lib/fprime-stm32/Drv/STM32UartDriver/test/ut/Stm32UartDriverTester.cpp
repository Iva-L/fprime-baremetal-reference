// ======================================================================
// \title  Stm32UartDriverTester.cpp
// \author ivanlara
// \brief  cpp file for Stm32UartDriver component test harness implementation class
// ======================================================================

#include "Stm32UartDriverTester.hpp"

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

Stm32UartDriverTester ::Stm32UartDriverTester()
    : Stm32UartDriverGTestBase("Stm32UartDriverTester", Stm32UartDriverTester::MAX_HISTORY_SIZE),
      component("Stm32UartDriver") {
    this->initComponents();
    this->connectPorts();
}

Stm32UartDriverTester ::~Stm32UartDriverTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Stm32UartDriverTester ::testOpenSuccess() {
    const Fw::Success status = this->component.open(64, 0, 0, 0, 115200);
    ASSERT_EQ(status, Fw::Success::SUCCESS);
    ASSERT_EVENTS_PortOpened_SIZE(1);
    ASSERT_EVENTS_PortOpened(0, 115200);
    ASSERT_from_ready_SIZE(1);
}

void Stm32UartDriverTester ::testSendFits() {
    (void)this->component.open(64, 0, 0, 0, 115200);

    U8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    const Drv::ByteStreamStatus status = this->invoke_to_send(0, buffer);
    ASSERT_EQ(status, Drv::ByteStreamStatus::OP_OK);
    ASSERT_EVENTS_TxRingFull_SIZE(0);
}

void Stm32UartDriverTester ::testSendRejectedWhenFull() {
    (void)this->component.open(64, 0, 0, 0, 115200);

    // TX_RING_SIZE is 4096 (UartDriverConfig.hpp) -- two 4000-byte sends
    // cannot both fit.
    static U8 data[4000] = {0};
    Fw::Buffer first(data, sizeof(data));
    ASSERT_EQ(this->invoke_to_send(0, first), Drv::ByteStreamStatus::OP_OK);

    Fw::Buffer second(data, sizeof(data));
    const Drv::ByteStreamStatus status = this->invoke_to_send(0, second);
    ASSERT_EQ(status, Drv::ByteStreamStatus::SEND_RETRY);
    ASSERT_EVENTS_TxRingFull_SIZE(1);
}

void Stm32UartDriverTester ::testPollDrainsTx() {
    (void)this->component.open(64, 0, 0, 0, 115200);

    U8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    ASSERT_EQ(this->invoke_to_send(0, buffer), Drv::ByteStreamStatus::OP_OK);

    this->component.poll();  // hwStartTx() (stubbed) reports success

    this->invoke_to_run(0, 0);
    ASSERT_TLM_BytesSent_SIZE(1);
    ASSERT_TLM_BytesSent(0, 8u);
}

void Stm32UartDriverTester ::testPollDrainsRx() {
    (void)this->component.open(64, 0, 0, 0, 115200);
    this->m_allocateReturnsValid = true;

    // No real DMA on the host to fill m_rxStaging -- stage the bytes
    // directly (Stm32UartDriverTester is a friend for exactly this).
    const FwSizeType len = 4;
    this->component.m_rxStaging[0] = 0xAA;
    this->component.m_rxStaging[1] = 0xBB;
    this->component.m_rxStaging[2] = 0xCC;
    this->component.m_rxStaging[3] = 0xDD;

    this->component.signalRxChunk(len);
    this->component.poll();

    ASSERT_from_recv_SIZE(1);
    const auto& recvEntry = this->fromPortHistory_recv->at(0);
    ASSERT_EQ(recvEntry.buffer.getData(), this->m_allocateBacking);
    ASSERT_EQ(recvEntry.buffer.getSize(), len);
    ASSERT_EQ(recvEntry.status, Drv::ByteStreamStatus::OP_OK);
    ASSERT_EQ(this->m_allocateBacking[0], 0xAA);
    ASSERT_EQ(this->m_allocateBacking[1], 0xBB);
    ASSERT_EQ(this->m_allocateBacking[2], 0xCC);
    ASSERT_EQ(this->m_allocateBacking[3], 0xDD);

    this->invoke_to_run(0, 0);
    ASSERT_TLM_BytesRecv_SIZE(1);
    ASSERT_TLM_BytesRecv(0, len);
}

void Stm32UartDriverTester ::testPollRxNoBuffers() {
    (void)this->component.open(64, 0, 0, 0, 115200);
    this->m_allocateReturnsValid = false;  // matches the GTestBase default, spelled out for clarity

    this->component.signalRxChunk(4);
    this->component.poll();

    ASSERT_from_recv_SIZE(0);
    ASSERT_EVENTS_NoBuffers_SIZE(1);
}

void Stm32UartDriverTester ::testUartErrorRecovery() {
    (void)this->component.open(64, 0, 0, 0, 115200);

    this->component.signalUartError(0x08U);  // mirrors HAL_UART_ERROR_ORE, an RX-affecting error
    this->component.poll();

    ASSERT_EVENTS_UartError_SIZE(1);
    ASSERT_EVENTS_UartError(0, 0x08U);

    this->invoke_to_run(0, 0);
    ASSERT_TLM_RxErrorCount_SIZE(1);
    ASSERT_TLM_RxErrorCount(0, 1u);
    ASSERT_TLM_TxErrorCount(0, 0u);
}

Fw::Buffer Stm32UartDriverTester ::from_allocate_handler(FwIndexType portNum, FwSizeType size) {
    this->pushFromPortEntry_allocate(size);
    if (!this->m_allocateReturnsValid) {
        return Fw::Buffer();
    }
    FW_ASSERT(size <= sizeof(this->m_allocateBacking), static_cast<FwAssertArgType>(size));
    return Fw::Buffer(this->m_allocateBacking, size);
}

}  // namespace Stm32
