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

void Stm32UartDriverTester ::toDo() {
    // TODO
}

}  // namespace Stm32
