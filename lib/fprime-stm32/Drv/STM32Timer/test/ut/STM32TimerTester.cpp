// ======================================================================
// \title  STM32TimerTester.cpp
// \author ivanlara
// \brief  cpp file for STM32Timer component test harness implementation class
// ======================================================================

#include "STM32TimerTester.hpp"

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

STM32TimerTester ::STM32TimerTester()
    : STM32TimerGTestBase("STM32TimerTester", STM32TimerTester::MAX_HISTORY_SIZE), component("STM32Timer") {
    this->initComponents();
    this->connectPorts();
}

STM32TimerTester ::~STM32TimerTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void STM32TimerTester ::testOpenArms() {
    this->component.open(1000);
    ASSERT_EVENTS_Configured_SIZE(1);
    ASSERT_EVENTS_Configured(0, 1000);
    ASSERT_EQ(this->component.m_nextTarget, 1000u);
}

void STM32TimerTester ::testPollWithoutTick() {
    this->component.open(1000);
    this->component.poll();
    ASSERT_from_CycleOut_SIZE(0);
    ASSERT_TLM_TickCount_SIZE(0);
}

void STM32TimerTester ::testPollWithTick() {
    this->component.open(1000);
    this->component.signalTick();
    this->component.poll();
    ASSERT_from_CycleOut_SIZE(1);
    ASSERT_TLM_TickCount_SIZE(1);
    ASSERT_TLM_TickCount(0, 1);
    ASSERT_EVENTS_TickOverrun_SIZE(0);
}

void STM32TimerTester ::testOverrunDetection() {
    this->component.open(100);
    // Advance the fake counter well past the armed compare target so poll()
    // detects the target already elapsed.
    this->component.m_stubCounter = 500;
    this->component.signalTick();
    this->component.poll();
    ASSERT_EVENTS_TickOverrun_SIZE(1);
    ASSERT_TLM_OverrunCount_SIZE(1);
    ASSERT_TLM_OverrunCount(0, 1);
}

}  // namespace Stm32
