// ======================================================================
// \title  STM32Timer.hpp
// \author ivanlara
// \brief  hpp file for the STM32H7 hardware tick source (TIM2 CH2 output
//         compare), used to drive Svc::RateGroupDriver::CycleIn.
// ======================================================================

#ifndef STM32_TIMER_HPP
#define STM32_TIMER_HPP

#include <lib/fprime-stm32/Drv/STM32Timer/STM32TimerComponentAc.hpp>

#include <Fw/Types/BasicTypes.hpp>

namespace Stm32 {

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
    void open(U32 periodUs);

    //! Poll/drain step: consumes the ISR-latched tick flag, fires CycleOut
    //! with a fresh Os::RawTime timestamp, reprograms the next compare
    //! target, and detects/recovers a target that already elapsed before it
    //! could be reprogrammed (see TickOverrun). Called every cyclic-executive
    //! pass from Main.cpp, same pattern as Stm32::Stm32UartDriver::poll().
    void poll();

    //! Latch a pending tick. Called by the real ISR callback trampoline
    //! (HAL_TIM_OC_DelayElapsedCallback, a free function with no user-context
    //! pointer) on the stm32h7 target; a unit test may also call this
    //! directly to simulate a tick firing, since no ISR exists on the host.
    void signalTick();

  private:
    // ----------------------------------------------------------------------
    // HAL boundary: the only methods allowed to touch TIM_HandleTypeDef/HAL
    // calls or the raw TIM2 registers. Implemented once against the real
    // HAL in STM32Timer.cpp (stm32h7 target only) and once as a
    // fixed-behavior stand-in in STM32TimerStub.cpp (host unit tests).
    // ----------------------------------------------------------------------

    //! Configure TIM2 channel 2 for output-compare "frozen" mode with the
    //! given initial compare target and start it in interrupt mode.
    void hwArmChannel(U32 target);

    //! Read the live TIM2 free-running counter.
    U32 hwReadCounter();

    //! Reprogram channel 2's compare target on the already-armed channel.
    void hwSetCompare(U32 target);

    U32 m_periodTicks;   //!< tick period in TIM2 counts (1 count == 1 us)
    U32 m_nextTarget;    //!< CCR2 value currently armed
    U32 m_tickCount;     //!< total ticks raised, mirrors TickCount telemetry
    U32 m_overrunCount;  //!< total overruns recovered, mirrors OverrunCount telemetry
    bool m_opened;

    //! Latched by signalTick(); consumed by poll().
    volatile bool m_tickPending;

    //! Host-only fake TIM2 counter, advanced by hwReadCounter()/hwSetCompare()
    //! in STM32TimerStub.cpp so poll()'s overrun-detection logic is exercised
    //! by a unit test without real hardware. Unused on the stm32h7 target.
    U32 m_stubCounter;

    friend class STM32TimerTester;
};

}  // namespace Stm32

#endif