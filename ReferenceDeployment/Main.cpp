// ======================================================================
// \title Main.cpp
// \brief Bare-metal cyclic executive entry point. Intended for use with the STM32F4xx series of microcontrollers.
// ======================================================================
#include <ReferenceDeployment/BootstrapAllocator.hpp>
#include <ReferenceDeployment/Top/ReferenceDeploymentTopology.hpp>
#include <ReferenceDeployment/Top/ReferenceDeploymentTopologyAc.hpp>

#include <Os/Os.hpp>
#include <Os/RawTime.hpp>
#include <fprime-baremetal/Os/TaskRunner/TaskRunner.hpp>
#include <main.h>

int main() {
    HAL_Init();
    Os::init();
    Os::Baremetal::TaskRunner& taskRunner = Os::Baremetal::TaskRunner::getSingleton();

    ReferenceDeployment::TopologyState inputs = {};
    ReferenceDeployment::setupTopology(inputs);
    ReferenceDeployment::lockBootstrapAllocator();

    U32 lastTick = HAL_GetTick();
    while (true) {
        const U32 currentTick = HAL_GetTick();
        if (currentTick != lastTick) {
            lastTick = currentTick;
            Os::RawTime cycleStart;
            (void)cycleStart.now();
            static_cast<Svc::RateGroupDriverComponentBase&>(ReferenceDeployment::rateGroupDriver)
                .CycleIn_handlerBase(0, cycleStart);
        }

        // This runs one non-blocking state-machine step for every registered active component.
        taskRunner.runAll();
    }
}
