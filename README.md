# Fprime Baremetal Reference (STM32H7 F´)

This project is an implementation of F´ on baremetal hardware, specifically for STM32H7 based microcontrollers.

The BaremetalReference is a reference to run F´ on embedded systems with hardware constraints. Therefore, this deployment only consists of basic/essential F´ components.

F´ (F Prime) is a component-driven framework that enables rapid development and deployment of spaceflight and other embedded software applications.
**Please Visit the F´ Website:** https://fprime.jpl.nasa.gov.

## STM32H7 build

The project uses the GNU Arm Embedded toolchain and the `stm32h7` F´ platform
for the STM32H753XI-EVAL2 target. The platform disables POSIX and socket
support, selects STM32-specific cooperative Task, critical-section Mutex,
interrupt-safe Queue, and TIM2 microsecond-resolution RawTime OSAL delegates,
and retains `fprime-baremetal` implementations for CPU, memory, and
MicroFs-backed file services.

```shell
source fprime-venv/bin/activate
fprime-util generate -f
fprime-util build -j"$(nproc)"
```

The `ReferenceDeployment` image has been flashed to and verified running on
the physical STM32H753XI-EVAL2 board: execution reaches `main()`, completes
topology setup, and runs the non-blocking cyclic executive continuously with
no assertion failures. A production-ready flight image still requires the
STM32 USART1 DMA driver and extended hardware validation (timer rollover,
sustained-run stability).

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

The C-level heap family is also locked down: `ReferenceDeployment/CMakeLists.txt` passes `-Wl,--wrap=malloc`, `--wrap=calloc`, `--wrap=realloc`, and `--wrap=free`, so every reference to those symbols in the final image (including from newlib internals) resolves to `__wrap_*` implementations in `ReferenceDeployment/MallocWrappers.cpp` instead of the real libc functions. `__wrap_malloc` forwards to the same bootstrap pool used by `operator new` (see "Hardware bring-up" below for why); `__wrap_calloc`, `__wrap_realloc`, and `__wrap_free` still immediately call `FW_ASSERT(0, ...)`, so any of those direct C heap calls traps at the point of use rather than silently allocating. Verified via `arm-none-eabi-nm`/objdump that `__wrap_malloc` and `__wrap_free` are linked; `__wrap_calloc`/`__wrap_realloc` are currently unreferenced and therefore garbage-collected by `--gc-sections` (they will be pulled in and enforced automatically the moment any code calls `calloc`/`realloc`).

### Hardware bring-up: first boot crash diagnosis and fix (August 31, 2026)

The first flash to the physical STM32H753XI-EVAL2 board did not reach `main()`: GDB/`pyocd` landed in `_exit.c` instead. Since no OpenOCD/GDB toolchain was available in this environment, the board was debugged directly using `pyocd` (installed via `pip`, no root required) with the exact `stm32h753xihx` CMSIS target pack, scripting `pyocd commander` sessions with breakpoints on `Reset_Handler`, `main`, `__wrap_malloc`, `__wrap_free`, `Fw::defaultSwAssert`, `abort`, and `_exit` to trace the real boot sequence.

Two distinct bugs were found and fixed:

1. **Premature `malloc()` before `main()`.** libstdc++'s exception-handling emergency pool (`eh_alloc.cc`, part of `libsupc++`) calls raw C `malloc(1088)` from a global constructor that runs during `__libc_init_array()` — strictly before `main()`, and therefore before `ReferenceDeployment::lockBootstrapAllocator()` is ever called. The prior `__wrap_malloc` unconditionally asserted on any call, so this toolchain-internal allocation tripped `FW_ASSERT` → `abort()` → `_exit()`, exactly matching the reported symptom. Fixed by routing `__wrap_malloc` through the existing `BootstrapAllocator` pool (the same one `OverrideNewDelete` already uses for `operator new`), which is open during the pre-lock bootstrap window and still asserts on any call after `lockBootstrapAllocator()`.
2. **`Os::Queue` was never implemented for bare-metal.** Only `Task`, `Mutex`, and `RawTime` had STM32 delegates; `cmake/platform/stm32h7.cmake` explicitly selected `Os_Queue_Stub` (F´'s no-op stub, whose `create()` always returns `UNKNOWN_ERROR`), so every active/queued component's queue creation — including `Svc::CommandDispatcher` — hard-faulted via `FW_ASSERT`. Fixed by implementing `lib/fprime-stm32/Os/Queue.{hpp,cpp}` (a single-threaded, `PRIMASK`-guarded, fixed-depth FIFO ring buffer allocated once from the bootstrap pool) and switching the platform config to `Os_Queue_Stm32`.

Both fixes were verified directly on the connected board via `pyocd`: the image now reaches `main()`, completes topology setup (including queue creation), and runs the cyclic executive loop continuously with zero hits on any malloc/assert/abort/exit breakpoint.

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
