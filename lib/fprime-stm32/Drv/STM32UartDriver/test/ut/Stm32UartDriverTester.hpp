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

    //! To do
    void toDo();

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
