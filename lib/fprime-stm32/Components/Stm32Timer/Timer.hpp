#ifndef STM32_TIMER_HPP
#define STM32_TIMER_HPP
#include <fprime-stm32/Components/Stm32Timer/TimerComponentAc.hpp>
namespace Stm32 {
class Timer final : public TimerComponentBase {
public:
  explicit Timer(const char* name = "") : TimerComponentBase(name) {}
};
}
#endif
