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

The project generates and compiles the migrated hardware sources and FPP boundaries. The STM32 `ReferenceDeployment` now links with the memory regions explicitly named `DTCM_RAM` (128 KiB), `AXI_SRAM` (512 KiB), and `FLASH` (2 MiB). The linker script also defines an aligned, `NOLOAD` `.dtcm_bss` section with `_sdtcm_bss` and `_edtcm_bss` boundary symbols; it is currently empty until framework state variables are assigned to it.

The verified STM32 deployment target is:

```shell
ninja -C build-fprime-stm32h7 ReferenceDeployment
```

The linker map places `.bss` at `0x240006e8` in AXI SRAM and `.dtcm_bss` at `0x20000000` in DTCM. Heap and stack remain in DTCM. The next phase is to add compiler placement attributes for CPU-only framework state while keeping USART1 DMA buffers in DMA-accessible AXI SRAM.

The development order has been intentionally revised so memory configuration precedes OSAL implementation. This establishes the target's actual resource contract before timing, task, queue, and synchronization primitives are finalized. The functional USART1 DMA adapter for PB14/PB15 and the cyclic-executive `main()` remain pending.

The personal progress checklist can be found in the [Checklist file](Checklist.md).
