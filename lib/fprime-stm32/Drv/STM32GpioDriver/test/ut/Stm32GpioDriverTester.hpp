// ======================================================================
// \title  Stm32GpioDriverTester.hpp
// \author ivanlara
// \brief  hpp file for Stm32GpioDriver component test harness implementation class
// ======================================================================

#ifndef Stm32_Stm32GpioDriverTester_HPP
#define Stm32_Stm32GpioDriverTester_HPP

#include "lib/fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriver.hpp"
#include "lib/fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriverGTestBase.hpp"

namespace Stm32 {

class Stm32GpioDriverTester final : public Stm32GpioDriverGTestBase {
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

    //! Construct object Stm32GpioDriverTester
    Stm32GpioDriverTester();

    //! Destroy object Stm32GpioDriverTester
    ~Stm32GpioDriverTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! open() as an OUTPUT succeeds and emits ConfigureSuccess
    void testOpenOutputSuccess();

    //! open() as an INPUT succeeds and emits ConfigureSuccess
    void testOpenInputSuccess();

    //! gpioWrite/gpioRead before open() both report NOT_OPENED
    void testAccessBeforeOpen();

    //! gpioWrite on a pin opened as INPUT reports INVALID_MODE
    void testWriteWrongMode();

    //! gpioRead on a pin opened as OUTPUT reports INVALID_MODE
    void testReadWrongMode();

    //! gpioWrite on a pin opened as OUTPUT reports OP_OK
    void testWriteAfterOpen();

    //! gpioRead on a pin opened as INPUT reports OP_OK
    void testReadAfterOpen();

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
    Stm32GpioDriver component;
};

}  // namespace Stm32

#endif
