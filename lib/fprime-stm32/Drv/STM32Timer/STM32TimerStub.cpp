// ======================================================================
// \title  STM32TimerStub.cpp
// \author ivanlara
// \brief  HAL boundary stand-in for host-native unit tests. No TIM2
//         hardware exists on the build host, so hwReadCounter()/
//         hwSetCompare() operate on a fake, test-injectable counter
//         instead -- no HAL/CMSIS include, no register access. Swapped in
//         for STM32Timer.cpp by Drv/STM32Timer/CMakeLists.txt's
//         register_fprime_ut().
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32Timer/STM32Timer.hpp>

namespace Stm32 {

void STM32Timer ::hwArmChannel(U32 target) {}

U32 STM32Timer ::hwReadCounter() {
    return this->m_stubCounter;
}

void STM32Timer ::hwSetCompare(U32 target) {}

}  // namespace Stm32