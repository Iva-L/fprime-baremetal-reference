// ======================================================================
// \title  STM32Timer.cpp
// \author ivanlara
// \brief  HAL boundary for the STM32H7 TIM CH2 tick source (real
//         hardware implementation, stm32h7 target only). This is the only
//         file in this driver allowed to include tim.h -- see
//         STM32TimerStub.cpp for the host unit-test stand-in.
// ======================================================================

#include <fprime-stm32/Drv/STM32Timer/STM32Timer.hpp>

#include "tim.h"

namespace {

//! Single-instance callback trampoline: the ISR callback is a free function
//! with no user-context pointer, and the topology only ever instantiates
//! one STM32Timer against one physical TIM peripheral.
Stm32::STM32Timer* s_instance = nullptr;

//! HAL handle resolved by hwSelectInstance(), used by every subsequent
//! hwArmChannel()/hwReadCounter()/hwSetCompare() call -- mirrors
//! Stm32UartDriver's s_huart/Stm32I2cDriver's s_hi2c.
TIM_HandleTypeDef* s_htim = nullptr;

//! Convert a HAL-free TimerInstance to the corresponding HAL handle.
TIM_HandleTypeDef* toHalHandle(Stm32::TimerInstance instance) {

    TIM_HandleTypeDef* deviceHandle = nullptr;
    
    switch (instance) {
        case Stm32::TimerInstance::Tim1:
            #if TIM1_INSTANCE
            deviceHandle = &htim1;
            #endif
            break;
        case Stm32::TimerInstance::Tim2:
            #if TIM2_INSTANCE
            deviceHandle = &htim2;
            #endif
            break;
        case Stm32::TimerInstance::Tim3:
            #if TIM3_INSTANCE
            deviceHandle = &htim3;
            #endif
            break;
        case Stm32::TimerInstance::Tim4:
            #if TIM4_INSTANCE
            deviceHandle = &htim4;
            #endif
            break;
        case Stm32::TimerInstance::Tim5:
            #if TIM5_INSTANCE
            deviceHandle = &htim5;
            #endif
            break;
        case Stm32::TimerInstance::Tim6:
            #if TIM6_INSTANCE
            deviceHandle = &htim6;
            #endif
            break;
        case Stm32::TimerInstance::Tim7:
            #if TIM7_INSTANCE
            deviceHandle = &htim7;
            #endif
            break;
        case Stm32::TimerInstance::Tim8:
            #if TIM8_INSTANCE
            deviceHandle = &htim8;
            #endif
            break;
        case Stm32::TimerInstance::Tim12:
            #if TIM12_INSTANCE
            deviceHandle = &htim12;
            #endif
            break;
        case Stm32::TimerInstance::Tim13:
            #if TIM13_INSTANCE
            deviceHandle = &htim13;
            #endif
            break;
        case Stm32::TimerInstance::Tim14:
            #if TIM14_INSTANCE
            deviceHandle = &htim14;
            #endif
            break;
        case Stm32::TimerInstance::Tim15:
            #if TIM15_INSTANCE
            deviceHandle = &htim15;
            #endif
            break;
        case Stm32::TimerInstance::Tim16:
            #if TIM16_INSTANCE
            deviceHandle = &htim16;
            #endif
            break;
        case Stm32::TimerInstance::Tim17:
            #if TIM17_INSTANCE
            deviceHandle = &htim17;
            #endif
            break;
        default:
            FW_ASSERT(false, static_cast<FwAssertArgType>(instance));
            break;
    }
    return deviceHandle;
}

}  // namespace

extern "C" void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim == s_htim && s_instance != nullptr) {
        s_instance->signalTick();
    }
}

namespace Stm32 {

void STM32Timer ::hwSelectInstance(TimerInstance instance) {
    TIM_HandleTypeDef* const halHandle = toHalHandle(instance);
    FW_ASSERT(halHandle != nullptr, static_cast<FwAssertArgType>(instance));

    FW_ASSERT(s_instance == nullptr);
    s_instance = this;

    s_htim = halHandle;
}

void STM32Timer ::hwArmChannel(U32 target) {
    TIM_OC_InitTypeDef ocConfig = {};
    ocConfig.OCMode = TIM_OCMODE_TIMING;  // "Frozen": compare-match interrupt only, no pin/output effect
    ocConfig.Pulse = target;
    ocConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    ocConfig.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_StatusTypeDef status = HAL_TIM_OC_ConfigChannel(s_htim, &ocConfig, TIM_CHANNEL_2);
    FW_ASSERT(status == HAL_OK, static_cast<FwAssertArgType>(status));

    status = HAL_TIM_OC_Start_IT(s_htim, TIM_CHANNEL_2);
    FW_ASSERT(status == HAL_OK, static_cast<FwAssertArgType>(status));
}

U32 STM32Timer ::hwReadCounter() {
    return s_htim->Instance->CNT;
}

void STM32Timer ::hwSetCompare(U32 target) {
    __HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_2, target);
}

}  // namespace Stm32
