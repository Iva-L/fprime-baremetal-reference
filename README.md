# Fprime Baremetal Reference (STM32H7 F´)

This project is an implementation of F´ on baremetal hardware, specifically for STM32H7 based microcontrollers.

The BaremetalReference is a reference to run F´ on embedded systems with hardware constraints. Therefore, this deployment only consists of basic/essential F´ components.

F´ (F Prime) is a component-driven framework that enables rapid development and deployment of spaceflight and other embedded software applications.
**Please Visit the F´ Website:** https://fprime.jpl.nasa.gov.

## STM32H7 build

The project uses the GNU Arm Embedded toolchain and the `stm32h7` F´ platform
for the STM32H753XI-EVAL2 target. The platform selects the cooperative
`fprime-baremetal` OS implementations and disables POSIX and socket support.

```shell
source fprime-venv/bin/activate
fprime-util generate stm32h7 --build-cache build-fprime-stm32h7
fprime-util build --build-cache build-fprime-stm32h7 -j"$(nproc)"
```

The current build validates the framework and bare-metal libraries. A
flashable image still requires the board startup code, linker script, cyclic
executive `main()`, and STM32 USART1 DMA driver.

## Current migration status

The CubeMX hardware foundation is now integrated under `lib/fprime-stm32/`. This includes the STM32H753 CMSIS device headers, selected HAL drivers, the 25 MHz HSE clock configuration, MSP initialization, interrupt handlers, GPIO/DMA/TIM/USART support, startup assembly, and the deployment linker script.

The project generates and compiles the migrated hardware sources and FPP boundaries. The final STM32 image is not yet linkable because the current ReferenceDeployment exceeds the configured AXI SRAM region by approximately 149 KiB. The next phase is therefore memory and framework tailoring: reduce the reference topology, tune queues/stacks/buffers, and distribute linker sections before implementing the remaining bare-metal OSAL behavior.

The development order has been intentionally revised so memory configuration precedes OSAL implementation. This establishes the target's actual resource contract before timing, task, queue, and synchronization primitives are finalized. The functional USART1 DMA adapter for PB14/PB15 and the cyclic-executive `main()` remain pending.

The personal progress checklist can be found in the [Checklist file](Checklist.md).
