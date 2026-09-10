// ======================================================================
// \title  STM32Timer.cpp
// \author ivanlara
// \brief  cpp file for the STM32H7 hardware tick source (TIM2 CH2 output
//         compare)
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32Timer/STM32Timer.hpp>

#include <Os/RawTime.hpp>

#include "tim.h"

namespace {

volatile bool s_tickPending = false;

}  // namespace


extern "C" void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM2) {
        s_tickPending = true;
    }
}

namespace Stm32 {

STM32Timer ::STM32Timer(const char* const compName)
    : STM32TimerComponentBase(compName),
      m_periodTicks(0),
      m_nextTarget(0),
      m_tickCount(0),
      m_overrunCount(0),
      m_opened(false) {}

STM32Timer ::~STM32Timer() {}

void STM32Timer ::open(U32 periodUs) {
    FW_ASSERT(periodUs > 0);

    this->m_periodTicks = periodUs;  // TIM2 runs at 1 MHz: 1 tick == 1 us
    s_tickPending = false;
    this->m_nextTarget = TIM2->CNT + this->m_periodTicks;

    TIM_OC_InitTypeDef ocConfig = {};
    ocConfig.OCMode = TIM_OCMODE_TIMING;  // "Frozen": compare-match interrupt only, no pin/output effect
    ocConfig.Pulse = this->m_nextTarget;
    ocConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    ocConfig.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_StatusTypeDef status = HAL_TIM_OC_ConfigChannel(&htim2, &ocConfig, TIM_CHANNEL_2);
    FW_ASSERT(status == HAL_OK, static_cast<FwAssertArgType>(status));

    status = HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_2);
    FW_ASSERT(status == HAL_OK, static_cast<FwAssertArgType>(status));

    this->m_opened = true;
    this->log_ACTIVITY_HI_Configured(periodUs);
}

void STM32Timer ::poll() {
    if (!this->m_opened || !s_tickPending) {
        return;
    }
    s_tickPending = false;

    this->m_tickCount++;
    this->tlmWrite_TickCount(this->m_tickCount);

    Os::RawTime timestamp;
    (void)timestamp.now();
    this->CycleOut_out(0, timestamp);

    U32 nextTarget = this->m_nextTarget + this->m_periodTicks;
    const U32 now = TIM2->CNT;

    if (static_cast<I32>(now - nextTarget) >= 0) {
        const U32 lateUs = now - nextTarget;
        nextTarget = now + this->m_periodTicks;
        this->m_overrunCount++;
        this->tlmWrite_OverrunCount(this->m_overrunCount);
        this->log_WARNING_HI_TickOverrun(lateUs);
    }

    this->m_nextTarget = nextTarget;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, this->m_nextTarget);
}

}  // namespace Stm32
