// ======================================================================
// \title  STM32Timer.cpp
// \author ivanlara
// \brief  HAL boundary for the STM32H7 TIM2 CH2 tick source (real
//         hardware implementation, stm32h7 target only). This is the only
//         file in this driver allowed to include tim.h -- see
//         STM32TimerStub.cpp for the host unit-test stand-in.
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32Timer/STM32Timer.hpp>

#include "tim.h"

namespace {

//! Single-instance callback trampoline: TIM2's ISR callback is a free
//! function with no user-context pointer, and the topology only ever
//! instantiates one STM32Timer against the one physical TIM2 peripheral.
Stm32::STM32Timer* s_instance = nullptr;

}  // namespace

extern "C" void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM2 && s_instance != nullptr) {
        s_instance->signalTick();
    }
}

namespace Stm32 {

void STM32Timer ::hwArmChannel(U32 target) {
    s_instance = this;

    TIM_OC_InitTypeDef ocConfig = {};
    ocConfig.OCMode = TIM_OCMODE_TIMING;  // "Frozen": compare-match interrupt only, no pin/output effect
    ocConfig.Pulse = target;
    ocConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    ocConfig.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_StatusTypeDef status = HAL_TIM_OC_ConfigChannel(&htim2, &ocConfig, TIM_CHANNEL_2);
    FW_ASSERT(status == HAL_OK, static_cast<FwAssertArgType>(status));

    status = HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_2);
    FW_ASSERT(status == HAL_OK, static_cast<FwAssertArgType>(status));
}

U32 STM32Timer ::hwReadCounter() {
    return TIM2->CNT;
}

void STM32Timer ::hwSetCompare(U32 target) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, target);
}

}  // namespace Stm32
