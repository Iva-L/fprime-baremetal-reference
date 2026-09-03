// ======================================================================
// \title  STM32Timer.hpp
// \author ivanlara
// \brief  hpp file for the STM32H7 hardware tick source (TIM2 CH2 output
//         compare), used to drive Svc::RateGroupDriver::CycleIn.
// ======================================================================

#ifndef STM32_TIMER_HPP
#define STM32_TIMER_HPP

#include <lib/fprime-stm32/Drv/STM32Timer/STM32TimerComponentAc.hpp>

#include <cstdint>

namespace Drv {

class STM32Timer final : public STM32TimerComponentBase {
  public:
    //! Construct object STM32Timer
    explicit STM32Timer(const char* const compName);

    //! Destroy object STM32Timer
    ~STM32Timer();

    //! Arm the TIM2 channel 2 output-compare tick source. Must be called once
    //! from configureTopology(), after Stm32_Tim2ClockInit() has already
    //! started TIM2's base counter (see Main.cpp) -- this only configures and
    //! starts channel 2 on top of the already-running counter.
    //! \param periodUs: tick period in microseconds (TIM2 runs at 1 MHz, so
    //!        this is a direct tick count with no conversion)
    void open(uint32_t periodUs);

    //! Poll/drain step: consumes the ISR-latched tick flag, fires CycleOut
    //! with a fresh Os::RawTime timestamp, reprograms the next compare
    //! target, and detects/recovers a target that already elapsed before it
    //! could be reprogrammed (see TickOverrun). Called every cyclic-executive
    //! pass from Main.cpp, same pattern as Drv::Stm32UartDriver::poll().
    void poll();

  private:
    uint32_t m_periodTicks;   //!< tick period in TIM2 counts (1 count == 1 us)
    uint32_t m_nextTarget;    //!< CCR2 value currently armed
    uint32_t m_tickCount;     //!< total ticks raised, mirrors TickCount telemetry
    uint32_t m_overrunCount;  //!< total overruns recovered, mirrors OverrunCount telemetry
    bool m_opened;
};

}  // namespace Drv

#endif