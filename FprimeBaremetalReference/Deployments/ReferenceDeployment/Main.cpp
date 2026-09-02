// ======================================================================
// \title Main.cpp
// \brief Bare-metal cyclic executive entry point with integrated hardware LED blinker.
// ======================================================================
#include <ReferenceDeployment/BootstrapAllocator.hpp>
#include <ReferenceDeployment/Top/ReferenceDeploymentTopology.hpp>
#include <ReferenceDeployment/Top/ReferenceDeploymentTopologyAc.hpp>

#include <Os/Os.hpp>
#include <Os/RawTime.hpp>
#include <fprime-baremetal/Os/TaskRunner/TaskRunner.hpp>
#include <main.h>
#include <tim2_clock.h>
#include "stm32h7xx_hal.h"

// GPIO Initialization for the LED on PF10
void Stm32_LedGpioInit() {

    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
}

int main() {
    HAL_Init();
    Stm32_Tim2ClockInit();
    
    // Initialize the LED hardware before entering the topology
    Stm32_LedGpioInit();
    
    Os::init();
    Os::Baremetal::TaskRunner& taskRunner = Os::Baremetal::TaskRunner::getSingleton();

    ReferenceDeployment::TopologyState inputs = {};
    ReferenceDeployment::setupTopology(inputs);
    ReferenceDeployment::lockBootstrapAllocator();

    U32 lastTick = HAL_GetTick();
    U32 blinkCounter = 0;

    while (true) {
        const U32 currentTick = HAL_GetTick();
        if (currentTick != lastTick) {
            const U32 elapsedTicks = currentTick - lastTick;
            lastTick = currentTick;

            blinkCounter += elapsedTicks;
            if (blinkCounter >= 1000) {
                HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_10);
                blinkCounter = 0;
            }

            Os::RawTime cycleStart;
            (void)cycleStart.now();
            static_cast<Svc::RateGroupDriverComponentBase&>(ReferenceDeployment::rateGroupDriver)
                .CycleIn_handlerBase(0, cycleStart);
        }

        // Run a cooperative state machine step for each active registered component.
        taskRunner.runAll();
    }
}