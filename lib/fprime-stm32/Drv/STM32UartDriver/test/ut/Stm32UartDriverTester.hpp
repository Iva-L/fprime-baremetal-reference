// ======================================================================
// \title  Stm32UartDriverTester.hpp
// \author ivanlara
// \brief  hpp file for Stm32UartDriver component test harness implementation class
// ======================================================================

#ifndef Stm32_Stm32UartDriverTester_HPP
#define Stm32_Stm32UartDriverTester_HPP

#include "lib/fprime-stm32/Drv/STM32UartDriver/Stm32UartDriver.hpp"
#include "lib/fprime-stm32/Drv/STM32UartDriver/Stm32UartDriverGTestBase.hpp"

namespace Stm32 {

class Stm32UartDriverTester final : public Stm32UartDriverGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object Stm32UartDriverTester
    Stm32UartDriverTester();

    //! Destroy object Stm32UartDriverTester
    ~Stm32UartDriverTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! open() succeeds, reports the actual baud, and emits PortOpened/ready
    void testOpenSuccess();

    //! send() enqueues into the TX ring and returns OP_OK
    void testSendFits();

    //! send() rejects a request that would overflow the TX ring
    void testSendRejectedWhenFull();

    //! poll() drains the TX ring into a (stubbed) DMA transfer, reflected
    //! in BytesSent telemetry on the next run() tick
    void testPollDrainsTx();

    //! signalRxChunk() + poll() drains staged bytes to recv() with a
    //! successfully allocated buffer
    void testPollDrainsRx();

    //! poll() reports NoBuffers and leaves data queued when allocate()
    //! cannot supply a buffer
    void testPollRxNoBuffers();

    //! signalUartError() + poll() recovers and counts the error
    void testUartErrorRecovery();

  private:
    // ----------------------------------------------------------------------
    // Test support
    // ----------------------------------------------------------------------

    //! Override: by default the base class returns an invalid Fw::Buffer;
    //! tests that need allocate() to succeed set m_allocateReturnsValid.
    Fw::Buffer from_allocate_handler(FwIndexType portNum, FwSizeType size) override;

    bool m_allocateReturnsValid = false;
    U8 m_allocateBacking[256] = {0};

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    Stm32UartDriver component;
};

}  // namespace Stm32

#endif
