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
no assertion failures, with the physical LED confirmed toggling via a live
GDB register poll. A production-ready flight image still requires the
STM32 USART1 DMA driver, extended hardware validation (timer rollover,
sustained-run stability), and revisiting the AXI SRAM memory budget (see
"Hardware bring-up" below — margin is now thin after a bootstrap-allocator
pool increase).

## Current migration status

The CubeMX hardware foundation is now integrated under `lib/fprime-stm32/`. This includes the STM32H753 CMSIS device headers, selected HAL drivers, the 25 MHz HSE clock configuration, MSP initialization, interrupt handlers, GPIO/DMA/TIM/USART support, startup assembly, and the deployment linker script.

The project generates and compiles the migrated hardware sources and FPP boundaries. The STM32 `ReferenceDeployment` now links with the memory regions explicitly named `DTCM_RAM` (128 KiB), `AXI_SRAM` (512 KiB), and `FLASH` (2 MiB). The linker script also defines an aligned, `NOLOAD` `.dtcm_bss` section with `_sdtcm_bss` and `_edtcm_bss` boundary symbols.

`lib/fprime-stm32/Os/` supplies the selected OSAL delegates. `Os::Task` is
cooperative and creates no thread, `Os::Mutex` preserves and restores the
Cortex-M7 `PRIMASK` around a bounded critical section, and `Os::RawTime`
assembles a race-safe 64-bit microsecond count from a free-running TIM2
timer (1 MHz, 32-bit up-counter) plus an interrupt-driven overflow counter,
serialized as seconds and microseconds. `FprimeBaremetalReference/Deployments/ReferenceDeployment/Main.cpp` now
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

The linker map places `.bss` at `0x240006e8` in AXI SRAM and `.dtcm_bss` at `0x20000000` in DTCM. `Svc::CommandDispatcher` and `Svc::PrmDb` state are routed to DTCM, while the bootstrap allocation pool (see "Hardware bring-up" below for its current 96 KiB size) remains in AXI SRAM for DMA accessibility. Heap and stack remain in DTCM.

The deployment depends on `Os_Baremetal_OverrideNewDelete`. Its global C++ `new` and `delete` overrides are registered before static constructors execute, then route allocations through the fixed bootstrap pool. Allocation is locked after topology initialization, causing a post-initialization allocation request to trigger an F´ assertion before cyclic execution begins.

The C-level heap family is also locked down: `FprimeBaremetalReference/Deployments/ReferenceDeployment/CMakeLists.txt` passes `-Wl,--wrap=malloc`, `--wrap=calloc`, `--wrap=realloc`, and `--wrap=free`, so every reference to those symbols in the final image (including from newlib internals) resolves to `__wrap_*` implementations in `FprimeBaremetalReference/Deployments/ReferenceDeployment/MallocWrappers.cpp` instead of the real libc functions. `__wrap_malloc` forwards to the same bootstrap pool used by `operator new` (see "Hardware bring-up" below for why); `__wrap_calloc`, `__wrap_realloc`, and `__wrap_free` still immediately call `FW_ASSERT(0, ...)`, so any of those direct C heap calls traps at the point of use rather than silently allocating. Verified via `arm-none-eabi-nm`/objdump that `__wrap_malloc` and `__wrap_free` are linked; `__wrap_calloc`/`__wrap_realloc` are currently unreferenced and therefore garbage-collected by `--gc-sections` (they will be pulled in and enforced automatically the moment any code calls `calloc`/`realloc`).

### Hardware bring-up: first boot crash diagnosis and fix (August 31, 2026)

The first flash to the physical STM32H753XI-EVAL2 board did not reach `main()`: GDB/`pyocd` landed in `_exit.c` instead. Since no OpenOCD/GDB toolchain was available in this environment, the board was debugged directly using `pyocd` (installed via `pip`, no root required) with the exact `stm32h753xihx` CMSIS target pack, scripting `pyocd commander` sessions with breakpoints on `Reset_Handler`, `main`, `__wrap_malloc`, `__wrap_free`, `Fw::defaultSwAssert`, `abort`, and `_exit` to trace the real boot sequence.

Two distinct bugs were found and fixed:

1. **Premature `malloc()` before `main()`.** libstdc++'s exception-handling emergency pool (`eh_alloc.cc`, part of `libsupc++`) calls raw C `malloc(1088)` from a global constructor that runs during `__libc_init_array()` — strictly before `main()`, and therefore before `ReferenceDeployment::lockBootstrapAllocator()` is ever called. The prior `__wrap_malloc` unconditionally asserted on any call, so this toolchain-internal allocation tripped `FW_ASSERT` → `abort()` → `_exit()`, exactly matching the reported symptom. Fixed by routing `__wrap_malloc` through the existing `BootstrapAllocator` pool (the same one `OverrideNewDelete` already uses for `operator new`), which is open during the pre-lock bootstrap window and still asserts on any call after `lockBootstrapAllocator()`.
2. **`Os::Queue` was never implemented for bare-metal.** Only `Task`, `Mutex`, and `RawTime` had STM32 delegates; `cmake/platform/stm32h7.cmake` explicitly selected `Os_Queue_Stub` (F´'s no-op stub, whose `create()` always returns `UNKNOWN_ERROR`), so every active/queued component's queue creation — including `Svc::CommandDispatcher` — hard-faulted via `FW_ASSERT`. Fixed by implementing `lib/fprime-stm32/Os/Queue.{hpp,cpp}` (a single-threaded, `PRIMASK`-guarded, fixed-depth FIFO ring buffer allocated once from the bootstrap pool) and switching the platform config to `Os_Queue_Stm32`.

Both fixes were verified directly on the connected board via `pyocd`: the image now reaches `main()`, completes topology setup (including queue creation), and runs the cyclic executive loop continuously with zero hits on any malloc/assert/abort/exit breakpoint.

### Hardware bring-up: ISR linking, bootstrap pool sizing, and time-source fixes (September 1, 2026)

A simplified LED-blinker test build (`Main.cpp` with `setupTopology()` commented out) ran for a while past `HAL_GetTick()` and then landed in `Default_Handler`'s infinite loop; re-enabling `setupTopology()` initially crashed even earlier. Live ST-LINK/GDB hardware debugging (`ST-LINK_gdbserver --persistent` + `arm-none-eabi-gdb`) uncovered four distinct bugs:

1. **Real ISR handlers were silently discarded by the linker.** `nm -A` showed `SysTick_Handler`, `HardFault_Handler`, `NMI_Handler`, `DMA1_Stream0/1_IRQHandler`, and `WWDG_IRQHandler` all sharing the same address as `Default_Handler` (all weak). GNU `ld` only pulls an `.o` out of a static archive when something already linked has an *undefined* reference into it; since the startup file (linked directly, not archived) already supplies a weak `Default_Handler` alias for every vector, `stm32h7xx_it.c.o`'s real, strong handlers inside `libFprimeStm32.a` were never extracted. Fixed by moving `stm32h7xx_it.c` out of the archived library and into `register_fprime_deployment(SOURCES ...)`, so it links directly like `Main.cpp`. A latent duplicate `TIM2_IRQHandler` definition in `Main.cpp` was also removed to avoid a symbol clash once the archive issue was fixed.
2. **A recursive fatal-assert loop from skipping `setupTopology()`.** With `setupTopology()` commented out for the LED-only test, `Svc::EventManager`'s internal queue was never created, so any assertion (including the fatal-adapter's own attempt to log the assertion as an event) recursed back into the same uninitialized queue until `abort()`. Fixed by re-enabling `setupTopology()`.
3. **Bootstrap allocator pool exhaustion.** Re-enabling `setupTopology()` revealed the fixed 16 KiB `BootstrapAllocator` pool was too small for the full topology's queue/buffer allocations (`ComQueue`, `FileDownlink`, etc.). Fixed by raising `STATIC_HEAP_POOL_SIZE` to 96 KiB.
4. **`Svc::ChronoTime` relies on an unimplemented clock.** `std::chrono::system_clock::now()` has no real-time-clock backing on bare-metal newlib and returned a garbage timestamp, tripping `Fw::Time::set()`'s range assertion. Fixed by swapping `chronoTime: Svc.ChronoTime` for `osTime: Svc.OsTime` in the topology, which is driven by the project's own TIM2-backed `Os::RawTime` delegate.

All four fixes were verified live on hardware: the board runs the cyclic executive continuously with zero hits on `_exit`/`abort`/`HardFault_Handler`/`FatalReceive_handler` breakpoints, and repeated GDB reads of `GPIOF_ODR` (`0x58021414`) show bit 10 toggling between set and clear, confirming the LED physically blinks with the full `ReferenceDeployment` topology active. Note that the AXI SRAM `.bss` margin is now thin (~9%) after the bootstrap-pool increase; the memory-tuning plan for `config/FpConfig.h` queue depths/serialization sizes should be revisited before adding further components.

### Hardware bring-up: MicroFs initialization and TaskRunner dispatch-table corruption fixes (September 2, 2026)

Uncommenting `taskRunner.runAll();` in `Main.cpp` — the step that begins actually dispatching every registered active component's message queue rather than just ticking the rate-group driver — surfaced two more bugs, both diagnosed via the same live ST-LINK/GDB workflow:

1. **`Os::Baremetal::MicroFs` was never initialized.** A `bt full` showed `Svc::SystemResources::PhysMem()` (invoked periodically once `rateGroup_1Hz` began being dispatched) calling `Os::FileSystem::getFreeSpace("/")`, which hard-asserts because `Os::Baremetal::MicroFs::MicroFsInit()` had never been called anywhere in the project — nothing in the framework calls it automatically. Fixed by adding a `MicroFsInit()` call (a conservative static 2-bin config: 2×1024-byte files + 1×4096-byte file) at the top of `configureTopology()` in `FprimeBaremetalReference/Deployments/ReferenceDeployment/Top/ReferenceDeploymentTopology.cpp`, drawing its storage from the existing 96 KiB bootstrap pool (only ~88 bytes of new static `.bss` for the config struct itself). Note: `MicroFs` only recognizes `/bin<N>/file<M>`-style paths, so the literal paths used by `Svc::PrmDb`/`FileDownlink`/`FileUplink`/`DpCatalog` will currently resolve to "file not found" rather than crash — real parameter/file persistence is a follow-up item.
2. **A real bug in the `fprime-baremetal` framework: `Os::Baremetal::TaskRunner::addTask()`'s insertion sort was broken.** After fixing #1, a *different* FATAL appeared: `Svc::TlmChanComponentBase::Run_handlerBase` asserting `qStatus == Os::Queue::OP_OK` with `FULL` — `CdhCore::tlmSend`'s own dispatch queue was never being drained. A GDB dump of `TaskRunner::m_task_table` showed clear duplicate entries (several components appearing twice) while several others — including `CdhCore::tlmSend` — were completely absent. The existing `addTask()` code wrote the new task directly into the tail slot and *then* ran a separate swap-based "sort" loop starting from index 0 with the same task re-used as the sort element — a broken pattern that both duplicates and drops table entries. A standalone Python simulation of the exact algorithm, fed the project's real task registration order and priorities, reproduced the identical corrupted table seen on hardware (7 of 17 active components dropped), conclusively proving the root cause: those components' internal queues were never dispatched, so any message sent to them (like the periodic `RUN_SCHED` sent to `tlmSend`) simply piled up until it overflowed. Fixed by replacing the broken logic in `lib/fprime-baremetal/fprime-baremetal/Os/TaskRunner/TaskRunner.cpp` with a correct O(n) shift-based insertion sort; re-running the simulation against the corrected algorithm confirmed all 17 tasks are retained exactly once and properly sorted by descending priority.

Both fixes were verified live on hardware with `taskRunner.runAll()` fully enabled: the board ran continuously for 2+ minutes with zero hits on `_exit`/`abort`/`HardFault_Handler`/`FatalReceive_handler` breakpoints (confirmed via repeated `interrupt`/`bt`/`detach` polling cycles showing normal application code, e.g. `Svc::ActiveRateGroup::CycleIn_handler`), and repeated `GPIOF_ODR` reads confirmed the LED still toggling (`0x400` ↔ `0x0`) under the full active-component dispatch load — the entire `ReferenceDeployment` topology, including every active component's cooperative dispatch, now runs cleanly end-to-end on the physical board.

### Memory optimization for USART1 DMA integration (September 2, 2026)

After the bootstrap allocator was increased to 96 KiB to support the complete topology, the deployment's AXI SRAM margin was too small for the planned USART1 DMA ground-link driver. `baremetal-size stm32h7` identified `CdhCore::tlmSend` as the dominant consumer: the framework-default `TLMCHAN_HASH_BUCKETS = 500` reserved 320,872 bytes for a deployment containing only 93 telemetry channels.

The project configuration now overrides the relevant framework configuration headers through `config/CMakeLists.txt`:

| Configuration | Previous value | Optimized value | Rationale |
|---|---:|---:|---|
| `TLMCHAN_HASH_BUCKETS` | 500 | 128 | Supports 93 current telemetry channels with approximately 37% capacity headroom |
| `TLMCHAN_NUM_TLM_HASH_SLOTS` | 15 | 41 | Provides a balanced hash table for the current telemetry-producing components |
| `CMD_DISPATCHER_DISPATCH_TABLE_SIZE` | 150 | 64 | Supports 48 current command opcodes with room for additional driver commands |
| `CMD_DISPATCHER_SEQUENCER_TABLE_SIZE` | 25 | 16 | Retains bounded command-sequencer capacity for the single-threaded deployment |
| `PRMDB_NUM_DB_ENTRIES` | 25 | 8 | Provides near-term parameter headroom; no parameters are currently declared |
| `DP_MAX_FILES` | 127 | 16 | Matches the small MicroFs-backed deployment while retaining file-tracking headroom |

The optimized overrides are implemented in `config/TlmChanImplCfg.hpp`, `config/CommandDispatcherImplCfg.hpp`, `config/PrmDbImplCfg.hpp`, and `config/DpCatalogCfg.hpp`. `config/FpConfig.h` and `config/FpConstants.fpp` already contained the deployment's reduced logging, string, serialization, and communication-buffer settings, so no additional changes were required there.

The optimized image was rebuilt and flashed successfully to the physical STM32H753XI-EVAL2 board. The `baremetal-size stm32h7` results show the following improvement:

| Region | Before optimization | After optimization | Improvement |
|---|---:|---:|---:|
| AXI SRAM `.bss` | 487,524 bytes | 226,988 bytes | 260,536 bytes recovered |
| AXI SRAM usage | 93.0% | 43.3% | Margin increased to approximately 56.7% |
| DTCM `.dtcm_bss` | 21,032 bytes | 8,584 bytes | 12,448 bytes recovered |
| Flash image | approximately 559 KiB | approximately 559 KiB | Essentially unchanged |
| `CdhCore::tlmSend` | 320,872 bytes | approximately 83,000 bytes | approximately 238 KiB recovered |

The deployment therefore has sufficient AXI SRAM headroom for USART1 PB14/PB15 DMA buffers, fixed-size UART ring buffers, and the initial ground-link integration without increasing the bootstrap pool again.

### Sizing and memory baseline

Memory baseline was verified on August 27, 2026, using the modified `baremetal-size` utility for the STM32H753XI platform:

| Region | Used | Capacity | Remaining margin |
|---|---:|---:|---:|
| Flash | 551,488 bytes (538.5 KiB) | 2,048 KiB | 73.1% |
| AXI SRAM (`.bss`) | 395,364 bytes | 512 KiB | 25.1% |
| DTCM RAM (`.dtcm_bss`) | 21,688 bytes | 128 KiB | 83.1% |

This confirms that the linker segmentation moved the CPU-only `CdhCore::cmdDisp` (`Svc::CommandDispatcher`) and `FileHandling::prmDb` (`Svc::PrmDb`) state into DTCM, reclaiming approximately 21.6 KiB of DMA-safe AXI SRAM headroom. The static memory contract is enforced by the AXI-SRAM bootstrap pool (16 KiB at the time of this baseline; raised to 96 KiB on September 1, 2026 — see "Hardware bring-up" above), post-initialization allocator locking with `FW_ASSERT(!m_locked)`, and GNU linker traps for direct C heap calls.

Updated `baremetal-size stm32h7` totals after the September 2, 2026 memory optimization (full `ReferenceDeployment` topology, `setupTopology()` + `taskRunner.runAll()` both enabled):

| Region | Used | Capacity | Remaining margin |
|---|---:|---:|---:|
| Flash (`.text`+`.rodata`+`.data`) | 560,160 bytes (547.0 KiB) | 2,048 KiB | 73.3% |
| AXI SRAM (`.bss`) | 226,988 bytes | 512 KiB (524,288 bytes) | 56.7% |
| DTCM RAM (`.dtcm_bss`) | 8,584 bytes | 128 KiB | 93.5% |


The development order has been intentionally revised so memory configuration precedes OSAL implementation. This established the target resource contract before finalizing the Task, Mutex, Queue, and RawTime delegation path. The cyclic-executive main loop, a TIM2-backed microsecond RawTime clock, and a bare-metal `Os::Queue` delegate are now implemented, linked, and confirmed running on the physical board, with the LED physically verified blinking end-to-end — including with every active component's cooperative dispatch fully enabled via `taskRunner.runAll()` (see "Hardware bring-up" above). The next step is implementing the non-blocking USART1 DMA adapter for PB14/PB15 using the recovered AXI SRAM budget, followed by extended TIM2 rollover, interrupt-mask, and sustained-run validation.

The personal progress checklist can be found in the [Checklist file](Checklist.md).
