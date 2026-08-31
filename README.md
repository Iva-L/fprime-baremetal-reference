# Fprime Baremetal Reference (STM32H7 F´)

This project is an implementation of F´ on baremetal hardware, specifically for STM32H7 based microcontrollers.

The BaremetalReference is a reference to run F´ on embedded systems with hardware constraints. Therefore, this deployment only consists of basic/essential F´ components.

F´ (F Prime) is a component-driven framework that enables rapid development and deployment of spaceflight and other embedded software applications.
**Please Visit the F´ Website:** https://fprime.jpl.nasa.gov.

## STM32H7 build

The project uses the GNU Arm Embedded toolchain and the `stm32h7` F´ platform
for the STM32H753XI-EVAL2 target. The platform disables POSIX and socket
support, selects STM32-specific cooperative Task, critical-section Mutex, and
TIM2 microsecond-resolution RawTime OSAL delegates, and retains
`fprime-baremetal` implementations for CPU, memory, and MicroFs-backed file
services.

```shell
source fprime-venv/bin/activate
fprime-util generate -f
fprime-util build -j"$(nproc)"
```

The current build validates the framework and bare-metal libraries and links
a complete `ReferenceDeployment` image, including a non-blocking cyclic
executive `main()`. A flashable, hardware-verified image still requires the
STM32 USART1 DMA driver and physical bring-up/validation on the board.

## Current migration status

The CubeMX hardware foundation is now integrated under `lib/fprime-stm32/`. This includes the STM32H753 CMSIS device headers, selected HAL drivers, the 25 MHz HSE clock configuration, MSP initialization, interrupt handlers, GPIO/DMA/TIM/USART support, startup assembly, and the deployment linker script.

The project generates and compiles the migrated hardware sources and FPP boundaries. The STM32 `ReferenceDeployment` now links with the memory regions explicitly named `DTCM_RAM` (128 KiB), `AXI_SRAM` (512 KiB), and `FLASH` (2 MiB). The linker script also defines an aligned, `NOLOAD` `.dtcm_bss` section with `_sdtcm_bss` and `_edtcm_bss` boundary symbols.

`lib/fprime-stm32/Os/` supplies the selected OSAL delegates. `Os::Task` is
cooperative and creates no thread, `Os::Mutex` preserves and restores the
Cortex-M7 `PRIMASK` around a bounded critical section, and `Os::RawTime`
assembles a race-safe 64-bit microsecond count from a free-running TIM2
timer (1 MHz, 32-bit up-counter) plus an interrupt-driven overflow counter,
serialized as seconds and microseconds. `ReferenceDeployment/Main.cpp` now
implements the non-blocking cyclic executive: after `HAL_Init()` and
`Stm32_Tim2ClockInit()`, the loop triggers `RateGroupDriver::CycleIn_handlerBase()`
once per observed millisecond boundary and calls
`Os::Baremetal::TaskRunner::runAll()` every pass, which runs one cooperative
state-machine step for every registered active component (including
`CdhCore::cmdDisp` and `ReferenceDeployment::cmdSeq`) with no threads, delays,
or blocking waits.

The verified STM32 deployment target is:

```shell
ninja -C build-fprime-stm32h7 ReferenceDeployment
```

The linker map places `.bss` at `0x240006e8` in AXI SRAM and `.dtcm_bss` at `0x20000000` in DTCM. `Svc::CommandDispatcher` and `Svc::PrmDb` state are routed to DTCM, while the 16 KiB fixed bootstrap allocation pool remains in AXI SRAM for DMA accessibility. Heap and stack remain in DTCM.

The deployment depends on `Os_Baremetal_OverrideNewDelete`. Its global C++ `new` and `delete` overrides are registered before static constructors execute, then route allocations through the fixed bootstrap pool. Allocation is locked after topology initialization, causing a post-initialization allocation request to trigger an F´ assertion before cyclic execution begins.

The C-level heap family is also locked down: `ReferenceDeployment/CMakeLists.txt` passes `-Wl,--wrap=malloc`, `--wrap=calloc`, `--wrap=realloc`, and `--wrap=free`, so every reference to those symbols in the final image (including from newlib internals) resolves to `__wrap_*` implementations in `ReferenceDeployment/MallocWrappers.cpp` instead of the real libc functions. Each wrapper immediately calls `FW_ASSERT(0, ...)`, so any direct C heap call traps at the point of use rather than silently allocating. Verified via `arm-none-eabi-nm`/objdump that `__wrap_malloc` and `__wrap_free` are linked and call `Fw::SwAssert`; `__wrap_calloc`/`__wrap_realloc` are currently unreferenced and therefore garbage-collected by `--gc-sections` (they will be pulled in and enforced automatically the moment any code calls `calloc`/`realloc`).

### Sizing and memory baseline

Memory baseline was verified on August 27, 2026, using the modified `baremetal-size` utility for the STM32H753XI platform:

| Region | Used | Capacity | Remaining margin |
|---|---:|---:|---:|
| Flash | 551,488 bytes (538.5 KiB) | 2,048 KiB | 73.1% |
| AXI SRAM (`.bss`) | 395,364 bytes | 512 KiB | 25.1% |
| DTCM RAM (`.dtcm_bss`) | 21,688 bytes | 128 KiB | 83.1% |

This confirms that the linker segmentation moved the CPU-only `CdhCore::cmdDisp` (`Svc::CommandDispatcher`) and `FileHandling::prmDb` (`Svc::PrmDb`) state into DTCM, reclaiming approximately 21.6 KiB of DMA-safe AXI SRAM headroom. The static memory contract is enforced by the 16 KiB AXI-SRAM bootstrap pool, post-initialization allocator locking with `FW_ASSERT(!m_locked)`, and GNU linker traps for direct C heap calls.

The development order has been intentionally revised so memory configuration precedes OSAL implementation. This established the target resource contract before finalizing the Task, Mutex, and RawTime delegation path. The cyclic-executive main loop and a TIM2-backed microsecond RawTime clock are now implemented and linked; the next step is hardware bring-up (flashing and scope/logic-analyzer validation of TIM2 timing, rollover, and interrupt-mask behavior), followed by the functional USART1 DMA adapter for PB14/PB15.

The personal progress checklist can be found in the [Checklist file](Checklist.md).
