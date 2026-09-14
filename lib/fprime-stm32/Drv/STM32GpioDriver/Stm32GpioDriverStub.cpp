// ======================================================================
// \title  Stm32GpioDriverStub.cpp
// \author ivanlara
// \brief  HAL boundary stand-in for host-native unit tests. No STM32
//         hardware exists on the build host, so every call here is a
//         fixed-behavior stub -- no HAL/CMSIS include, no register access.
//         Swapped in for Stm32GpioDriver.cpp by
//         Drv/STM32GpioDriver/CMakeLists.txt's register_fprime_ut().
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriver.hpp>

namespace Stm32 {

bool Stm32GpioDriver ::hwConfigurePin(GpioPort port, U16 pin, Fw::Direction mode, Fw::Logic defaultState) {
    return true;  // no hardware to misconfigure on host; always report success
}

bool Stm32GpioDriver ::hwReadPin(GpioPort port, U16 pin) {
    return false;
}

void Stm32GpioDriver ::hwWritePin(GpioPort port, U16 pin, bool high) {}

}  // namespace Stm32