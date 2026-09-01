#  F Prime Baremetal Reference Project
*Target Hardware:* **STM32H753XI-EVAL2** (ARM Cortex-M7, 2MB Flash, 1MB SRAM)
*Execution Model:* **Pure Bare-Metal Cyclic Executive (Single-Threaded Polling Loop)**
*Primary Mentors:* Kevin Ortega (Mentor), Jeff Levison (Group Supervisor)

## Current Status Snapshot
*Current phase:* **Physical LED blinker verified end-to-end with the FULL cyclic executive active (`setupTopology()` + `taskRunner.runAll()` both enabled). Two additional bugs surfaced only once every active component's queue began being dispatched — an un-initialized bare-metal RAM filesystem (`Os::Baremetal::MicroFs`) and a broken insertion-sort in `Os::Baremetal::TaskRunner::addTask()` that silently dropped 7 of 17 active components (including `CdhCore::tlmSend`) from the round-robin dispatch table — diagnosed via live ST-LINK/GDB hardware debugging and fixed; the board now runs for 2+ minutes continuously with zero FATAL hits and `GPIOF_ODR` bit 10 confirmed physically toggling.**
*What is completed:*
*   [x] Host environment and F´ toolchain baseline are established.
*   [x] Project repository was created and aligned around the bare-metal F´ pattern.
*   [x] ARM GNU toolchain for STM32H7 is configured.
*   [x] Custom `cmake/toolchain/stm32h7.cmake` and platform CMake files exist.
*   [x] `settings.ini` and project config integration are corrected for the bare-metal library path.
*   [x] `fprime-util generate stm32h7` succeeds.
*   [x] Cross-build of the F´ framework and bare-metal library baseline succeeds.
*   [x] CubeMX CMSIS/HAL migration, clock code, MSP, interrupts, startup assembly, and linker integration are present.
*   [x] `lib/fprime-stm32` is registered as a reusable STM32 HAL library.
*   [x] STM32 FPP timer and UART boundaries are generated and compile.
*   [x] Tailor the framework and memory map before implementing OSAL primitives.
*   [x] Board startup and linker script for STM32H753 memory layout.
*   [x] STM32-specific `Os::Task`, `Os::Mutex`, and `Os::RawTime` delegates are registered and selected by the `stm32h7` platform.
*   [x] STM32H7 cross-build succeeds with POSIX disabled and `Os_Task_Stm32`, `Os_Mutex_Stm32`, and `Os_RawTime_Stm32` linked.
*   [x] `ReferenceDeployment/Main.cpp` implements the non-blocking cyclic-executive loop: `HAL_Init()`, topology setup, per-millisecond `RateGroupDriver::CycleIn_handlerBase()` trigger, and `Os::Baremetal::TaskRunner::runAll()` to dispatch every registered active component (`CdhCore::cmdDisp`, `ReferenceDeployment::cmdSeq`) once per loop pass with no threads, delays, or blocking waits.
*   [x] `Os_RawTime_Stm32` upgraded from millisecond `HAL_GetTick()` resolution to a microsecond-resolution free-running clock backed by TIM2 (1 MHz, 32-bit up-counter), with an interrupt-driven overflow counter and a race-safe double-read 64-bit assembly (`Stm32_GetSystemMicroseconds()`), serialized as seconds/microseconds for F´ time services.
*   [x] `Os_Queue_Stm32` implemented (single-threaded, interrupt-safe FIFO ring buffer) and selected by the `stm32h7` platform in place of the previously-linked no-op `Os_Queue_Stub`.
*   [x] First physical boot achieved: diagnosed and fixed a premature libstdc++ `malloc()` call before `main()` (was tripping `FW_ASSERT`/`abort()`/`_exit()`) and the missing `Os::Queue` delegate; verified via `pyocd` that the image now reaches `main()`, completes topology setup, and runs the cyclic loop continuously on the real STM32H753XI-EVAL2 board with no assertion failures.
*   [x] Diagnosed and fixed a static-archive ISR-linking bug: `stm32h7xx_it.c`'s real `SysTick_Handler`, `HardFault_Handler`, `NMI_Handler`, `DMA1_Stream0/1_IRQHandler`, and `TIM2_IRQHandler` were silently discarded by the linker in favor of weak `Default_Handler` aliases from the startup file, because they lived inside a static archive nothing forced extraction of. Fixed by compiling `stm32h7xx_it.c` directly into the executable via `register_fprime_deployment(SOURCES ...)`; verified via `nm -A` that all are now strong (`T`) symbols at distinct addresses.
*   [x] Diagnosed and fixed a bootstrap-allocator pool exhaustion bug: `BootstrapAllocator.cpp`'s fixed 16 KiB static pool was too small for the full topology's queue/buffer allocations during `setupTopology()`, causing a recursive fatal-assert loop (`Svc::EventManager`/`Svc::FatalHandler` re-entering the same failure while trying to log it). Fixed by raising `STATIC_HEAP_POOL_SIZE` to 96 KiB (`.bss` now 476,244/524,288 bytes in AXI SRAM — roughly 9% margin remaining).
*   [x] Diagnosed and fixed a `Svc::ChronoTime` bug: it relied on `std::chrono::system_clock`, which is unimplemented on bare-metal newlib and produced a garbage epoch timestamp, tripping `Fw::Time::set()`'s range assertion. Replaced `chronoTime: Svc.ChronoTime` with `osTime: Svc.OsTime` in `instances.fpp`/`topology.fpp`, which is driven by the existing TIM2-backed `Os::RawTime` delegate instead.
*   [x] Verified live on hardware via ST-LINK GDB server (`ST-LINK_gdbserver` + `arm-none-eabi-gdb`): with all three fixes applied, the board runs the cyclic executive continuously with zero hits on `_exit`/`abort`/`HardFault_Handler`/`FatalReceive_handler` breakpoints, and polling `GPIOF_ODR` (`0x58021414`) shows bit 10 toggling between `0x0` and `0x400`, confirming the LED blinker works end-to-end on the physical STM32H753XI-EVAL2 board.
*   [x] Diagnosed and fixed a null bare-metal filesystem bug: enabling `taskRunner.runAll();` in `Main.cpp` caused `Svc::SystemResources::PhysMem()` (invoked periodically by `rateGroup_1Hz`) to hard-assert in `Os::Baremetal::MicroFs`/`FileSystem.cpp` because `MicroFsInit()` had never been called anywhere in the project. Fixed by adding a `Os::Baremetal::MicroFs::MicroFsInit(...)` call (2-bin config, drawn from the existing bootstrap pool) at the top of `configureTopology()` in `ReferenceDeployment/Top/ReferenceDeploymentTopology.cpp`; `.bss` grew by only ~88 bytes.
*   [x] Diagnosed and fixed a critical `fprime-baremetal` framework bug in `Os::Baremetal::TaskRunner::addTask()`: its priority-sort insertion loop wrote the new task at the tail slot *and then* ran a separate swap-based "sort" starting from index 0, which silently duplicated some tasks in the round-robin table while completely dropping others. A Python simulation using the project's real registration order/priorities reproduced the exact corrupted table observed on hardware (7 of 17 active components dropped, including `CdhCore::tlmSend`), proving these components' internal dispatch queues were never drained by `taskRunner.runAll()` — explaining the `Svc::TlmChan` `Run_handlerBase` queue-`FULL` assert (`TlmChanComponentAc.cpp:668`). Fixed by replacing the broken swap logic with a correct O(n) shift-based insertion sort in `lib/fprime-baremetal/fprime-baremetal/Os/TaskRunner/TaskRunner.cpp`; re-simulated to confirm all 17 tasks are now retained, unique, and sorted by descending priority.
*   [x] Verified live on hardware with `taskRunner.runAll()` enabled: the board ran continuously for 2+ minutes with zero hits on `_exit`/`abort`/`HardFault_Handler`/`FatalReceive_handler` breakpoints, and repeated `GPIOF_ODR` reads confirmed the LED still toggling (`0x400` ↔ `0x0`) under the full active-component dispatch load.

*What remains:*

*   [ ] AXI SRAM margin is now thin (~9%) after the bootstrap pool increase — revisit `config/FpConfig.h` queue depths/serialization buffer sizes (per the earlier memory-tuning plan) before adding new components.
*   [ ] `Os::Baremetal::MicroFs`'s strict `/bin<N>/file<M>` path-naming scheme does not match the literal file paths used by `Svc::PrmDb`, `Svc::FileDownlink`/`FileUplink`, or `Svc::DpCatalog` (e.g. `PrmDb.dat`) — these currently resolve to `Status::INVALID` (file-not-found) rather than crashing, but real parameter/file persistence is not yet functional and needs a path-mapping or `MicroFs` naming-convention fix.
*   [ ] Continued hardware validation: monotonic TIM2-microsecond timestamps, correct overflow/rollover behavior (~71.58 minutes), interrupt-mask restoration under the Mutex delegate, and that the cyclic executive's per-millisecond rate-group trigger and cooperative dispatch behave correctly over an extended run against real `HAL_GetTick()`/TIM2 timing.
*   [ ] GPIO/UART configuration for LED1/LED3 and USART1 PB14/PB15.
*   [ ] DMA-backed USART1 ground communication using the byte-stream model.
*   [ ] Full application topology and hardware-aware component integration.

## Week-by-Week Schedule & Task Checklist

### Block 1: F Prime STM32H7 Porting (Weeks 1-9)

#### Week 1: Board Bring-up & IT Onboarding
*Goal: Initialize host development environments, complete mandatory security training, and verify basic hardware operation.*
*   [x] **JPL IT Onboarding & Training:** Secure IT asset access, set up your JPL laptop, and complete mandatory IT/security training.
*   [x] **Local WSL Ubuntu Environment Setup:** Install the required dependencies for F´ on your local WSL Ubuntu partition.
*   [x] **Framework Verification:** Build and run the F´ **Hello World** and local **LED Blinker** tutorials on your host machine to confirm toolchain functionality.
*   [x] **Hardware Verification (Bring-up):** Connect your STM32H753XI-EVAL2 board to your host via USB Micro-B to the **CN23 STLINK-V3E port**. Verify board communication, update the debugger firmware, and verify jumper settings.
*   [x] **Baremetal Blinky Test:** Compile and flash a simple bare-metal LED-blinking program using STM32CubeIDE to verify that the target board is fully functional.

---

#### Week 2: Bare-metal Execution Model (Cyclic Executive) Learning
*Goal: Understand the dispatcher mechanics, establish the cross-compilation toolchain, and set up your repository.*
*   [x] **Deconstruct Dispatcher Mechanism:** Analyze how `fprime-baremetal-reference` handles task scheduling without POSIX threads. Study how the main loop manually invokes `doDispatch()` sequentially on active component queues.
*   [x] **Create Official Repository:** Initialize your **`fprime-baremetal-reference`** repository under the `fprime-community` GitHub organization.
*   [x] **Setup ARM GCC Cross-Compiler:** Configure your WSL Ubuntu environment with the bare-metal compiler `gcc-arm-none-eabi` and C++ standard libraries `libstdc++-arm-none-eabi-dev`.
*   [x] **Construct CMake Platform & Toolchain Files:** 
    *   Create `cmake/toolchain/stm32h7-toolchain.cmake` to target the **Cortex-M7 hardware FPU** (`-mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16`).
    *   Create `cmake/platform/stm32h7.cmake` setting `set(FPRIME_USE_BAREMETAL ON)` and disabling OS subsystems.
*   [x] **Integrate `fprime-baremetal` Submodule:** Add your fprime's baremetal repository as a git submodule and register it inside your `settings.ini` file.
*   [x] **Dry Build Verification:** Run `fprime-util generate stm32h7` and compile a skeleton deployment to prove the cross-compilation pipeline is flawless before adding hardware drivers.
*   [x] **Hardware Peripheral Configuration:** Configure the clock tree (utilizing the on-board **25 MHz crystal X1**) and pins inside **STM32CubeMX**:
    *   *LED Pins:* **PF10** (User LED1) and **PA4** (User LED3).
    *   *UART Pins:* **PB14** (TX) and **PB15** (RX), routing to the embedded STLINK Virtual COM Port.

---

#### Week 3: Framework Tailoring & Memory Configuration
*Goal: Establish the memory contract and reduce the F´ deployment to a viable STM32H753 image before OSAL work.*
*   [x] **Reduce ReferenceDeployment footprint:** Remove or defer host-oriented subtopologies and unused services.
*   [x] **Tune `FpConfig.h`:** Reduce queue, stack, telemetry, and buffer defaults for the bare-metal target.
*   [x] **Linker Script Segmentation / Map linker sections:** Distribute `.data` and `.bss` across DTCM, AXI SRAM, D2 SRAM, and D3 SRAM deliberately.
*   [x] **Linker Script Segmentation:** Distribute static allocations so that non-DMA variables are routed to DTCM, leaving AXI SRAM open for DMA communication buffers.
*   [x] **Section Zero-Initialization:** Clear `_sdtcm_bss` through `_edtcm_bss` in `Reset_Handler` before C++ constructors run.
*   [x] **Enforce the Zero Dynamic Memory Contract:** Route C++ `new` through a fixed bootstrap pool and lock allocation before cyclic execution; wrap `malloc`/`calloc`/`realloc`/`free` via `-Wl,--wrap` so any direct C-level heap call traps with `FW_ASSERT` instead of silently allocating.
*   [x] **Profiling & Sizing Validation:** Verified exact memory allocations and margins using the modified STM32H7 `baremetal-size` utility on August 27, 2026: 551,488 bytes Flash, 395,364 bytes AXI SRAM `.bss`, and 21,688 bytes DTCM `.dtcm_bss`.
*   [x] **Topology Cleanup:** Pruned the `textLogger` and socket-based transport connections to achieve a clean bare-metal topology build.

---

#### Week 4: Bare-metal OS Abstraction Layer (OSAL) Primitives
*Goal: Implement OSAL behavior against the established memory and execution contract.*
*   [x] **Deconstruct `Os` Library:** Map the standard `Os` namespace interface to the bare-metal implementation.
*   [x] **Implement Cooperative Task OSAL:** Register and select `Os_Task_Stm32`, which creates no thread and reports cooperative execution.
*   [x] **Implement Interrupt-Safe Mutexes:** Register and select `Os_Mutex_Stm32`, preserving/restoring Cortex-M7 `PRIMASK` for bounded critical sections.
*   [x] **Configure Custom `Os::RawTime` Implementation:** Register and select `Os_RawTime_Stm32`, initially using `HAL_GetTick()` and seconds/microseconds serialization.
*   [x] **Integrate cyclic dispatch and time validation:** Invoke queued-component/cooperative work from `main()` via `Os::Baremetal::TaskRunner::runAll()` and trigger `RateGroupDriver::CycleIn_handlerBase()` on each observed millisecond boundary; hardware validation of monotonic timestamps, rollover behavior, and interrupt-mask restoration remains pending a physical flash test.
*   [x] **Upgrade `Os::RawTime` to microsecond resolution:** Replaced the millisecond `HAL_GetTick()` backing with a TIM2-based free-running 1 MHz counter (`lib/fprime-stm32/src/tim2_clock.cpp`), an interrupt-driven 32-bit overflow counter incremented in `HAL_TIM_PeriodElapsedCallback()`, and a race-safe double-read 64-bit microsecond assembly, still serialized as seconds/microseconds.
*   [x] **Implement `Os::Queue` OSAL delegate:** Added `lib/fprime-stm32/Os/Queue.{hpp,cpp}` and `DefaultQueue.cpp` — a single-threaded, interrupt-safe (`PRIMASK`-guarded) fixed-depth FIFO ring buffer, backed by storage allocated once from the bootstrap pool during `create()`. Registered via `register_os_implementation(Queue Stm32 FprimeStm32)`; this had been missing entirely, silently falling back to F´'s no-op `Os::Stub::Queue`.

#### Hardware Bring-up: First Boot Crash Diagnosis and Fix (August 31, 2026)
*Goal: Diagnose and resolve the physical-board startup crash where GDB/pyocd never reached `main()` and instead landed in `_exit.c`.*
*   [x] **Stood up a hardware debug path without OpenOCD/GDB:** Installed `pyocd` (pip, no sudo required) inside `fprime-venv`, installed the exact `Keil.STM32H7xx_DFP` CMSIS pack to get precise `stm32h753xihx` target support (only generic H723/H743/H750/H7b0 targets ship built-in), and used `pyocd commander` scripted sessions (`-x <scriptfile>`) with breakpoints on `Reset_Handler`/`main`/`__wrap_malloc`/`__wrap_free`/`Fw::defaultSwAssert`/`abort`/`_exit` to trace the real boot sequence on the connected STM32H753XI-EVAL2 board.
*   [x] **Root-caused Bug #1 — premature `malloc()` before `main()`:** libstdc++'s exception-handling emergency pool (`eh_alloc.cc`, part of `libsupc++`) calls raw C `malloc(1088)` from a global constructor that runs during `__libc_init_array()`, strictly before `main()` and therefore before `ReferenceDeployment::lockBootstrapAllocator()`. The prior `__wrap_malloc` unconditionally `FW_ASSERT`'d on any call, tripping `abort()` → `_exit()` — exactly the "GDB lands in `_exit.c`" symptom originally reported.
*   [x] **Fixed `ReferenceDeployment/MallocWrappers.cpp`:** `__wrap_malloc` now forwards to the same `BootstrapAllocator` pool already used by `Os_Baremetal_OverrideNewDelete` for `operator new`, permitting this one legitimate toolchain-internal allocation during the pre-lock bootstrap window while still hard-asserting on any post-lock or application-level call.
*   [x] **Root-caused Bug #2 — `Os::Queue` never implemented for bare-metal:** With Bug #1 fixed, tracing further revealed `Svc::CommandDispatcher::init()`'s `createQueue()` call hit a **new** `FW_ASSERT` (`CommandDispatcherComponentAc.cpp`, `qStat == UNKNOWN_ERROR`). Root cause: `cmake/platform/stm32h7.cmake` explicitly `CHOOSES_IMPLEMENTATIONS Os_Queue_Stub`, so every active/queued component's `Os::Queue::create()` silently failed.
*   [x] **Fixed by implementing `Os_Queue_Stm32` (see OSAL item above) and updating `cmake/platform/stm32h7.cmake`** to select it instead of `Os_Queue_Stub`.
*   [x] **Verified live on hardware:** Rebuilt, reflashed via `pyocd flash --format elf`, and re-traced with `pyocd commander`. Execution now reaches `main()` cleanly and the CPU remains in the `Running` state through topology setup and into the cyclic loop for 2.5+ seconds continuously with zero hits on any malloc/assert/abort/exit breakpoint — confirming both bugs are resolved and the cyclic executive is live on the physical STM32H753XI-EVAL2 board.

---

#### Hardware Bring-up: ISR Linking, Bootstrap Allocator Sizing, and Time Source Fixes (September 1, 2026)
*Goal: Diagnose and resolve a simplified LED-blinker test (`Main.cpp` with `setupTopology()` commented out) that ran until `HAL_GetTick()` for a while and then fell into `Default_Handler`'s infinite loop, and get a fully-wired `ReferenceDeployment` (with `setupTopology()` enabled) running cleanly with the LED physically blinking.*
*   [x] **Stood up a live ST-LINK/GDB debug path:** Started `ST-LINK_gdbserver --persistent` (requires `-cp <dir-containing-STM32_Programmer_CLI>`) and drove it with `arm-none-eabi-gdb` batch scripts (`monitor reset` + `load` + breakpoints + `continue`), using a background-process `kill -INT <gdb-pid>` trick to interrupt a "running" target and capture its live state without a breakpoint.
*   [x] **Root-caused Bug #1 — real ISR handlers silently discarded by the linker:** `nm -A` on the built ELF showed `SysTick_Handler`, `HardFault_Handler`, `NMI_Handler`, `DMA1_Stream0/1_IRQHandler`, and `WWDG_IRQHandler` all sharing the exact same address as `Default_Handler` (all weak). GNU `ld` only extracts an `.o` from a static archive (`libFprimeStm32.a`) when something already-linked has an *undefined* reference into it; since `startup_stm32h753xx.s.obj` (linked directly) already supplies a weak `Default_Handler` alias for every vector, every vector slot was "satisfied" before the linker ever searched the archive, so `stm32h7xx_it.c.o`'s real, strong handler definitions were never pulled in at all. A prior attempt to fix this with `-Wl,-u,<symbol>` linker flags did not work, because the CMake-generated link line places object files before archives, so the forced-undefined marker gets weakly satisfied before archives are searched.
*   [x] **Fixed by moving `stm32h7xx_it.c` out of the archived `FprimeStm32` library** (`lib/fprime-stm32/CMakeLists.txt`) **and into `register_fprime_deployment(SOURCES ...)`** (`ReferenceDeployment/CMakeLists.txt`), so it is compiled directly into the executable like `Main.cpp`/`BootstrapAllocator.cpp`/the startup file, guaranteeing its strong symbols are always present to override the weak aliases. Also removed a latent duplicate `TIM2_IRQHandler` definition from `Main.cpp` that would have caused a link error once the archive issue was fixed. Verified via `nm -A` that all affected ISRs are now strong (`T`) at distinct addresses.
*   [x] **Root-caused Bug #2 — recursive fatal-assert loop from skipped `setupTopology()`:** A full `bt full` on a live hang in `_exit` showed `Svc::EventManagerComponentBase::loqQueue_internalInterfaceInvoke` asserting because its internal message queue was never created (`.init()`, which calls `Os::Queue::create()`, only runs inside `setupTopology()`). The assert routed through `Fw::defaultSwAssert` → `AssertFatalAdapter::reportAssert` → tried to log that very assertion as an event via the *same* uninitialized `EventManager` queue → asserted again, recursing until `abort()`. Fixed by re-enabling `ReferenceDeployment::setupTopology(inputs);` in `Main.cpp` (had been commented out for the simplified LED-blinker test) and re-enabling the LED toggle line.
*   [x] **Root-caused Bug #3 — bootstrap allocator pool exhaustion:** With `setupTopology()` re-enabled, a new `FW_ASSERT` fired in `BootstrapAllocator.cpp:41` (`allocationEnd <= m_poolEnd`) — the fixed 16 KiB static pool was too small for the full topology's component queue/buffer allocations (`ComQueue`, `FileDownlink`, etc.) during `initComponents()`/`configComponents()`. This produced the same recursive fatal-assert-loop symptom as Bug #2, but from allocator exhaustion rather than an uninitialized queue. Fixed by iteratively raising `STATIC_HEAP_POOL_SIZE` in `ReferenceDeployment/BootstrapAllocator.cpp` from 16 KiB → 96 KiB, rebuilding and re-flashing after each step until no allocation-site assert remained (final AXI SRAM `.bss` usage: 476,244 / 524,288 bytes, ~9% margin remaining).
*   [x] **Root-caused Bug #4 — `Svc::ChronoTime` relies on an unimplemented clock:** After Bugs #2/#3 were fixed, a new assert fired in `Fw::Time::set()` (`Fw/Time/Time.cpp:38`) with an out-of-range `useconds` value (`4294726827`, over the 999999 max). Root cause: `Svc::ChronoTime::timeGetPort_handler` calls `std::chrono::system_clock::now()`, which has no real-time-clock backing on bare-metal newlib and returns garbage. Fixed by swapping `chronoTime: Svc.ChronoTime` for `osTime: Svc.OsTime` in `ReferenceDeployment/Top/instances.fpp` and `topology.fpp` — `Svc::OsTime` is driven by the project's own TIM2-backed `Os::RawTime` delegate and safely returns `Fw::ZERO_TIME` until an epoch is set via command, rather than asserting.
*   [x] **Verified live on hardware:** With all four fixes applied, `ST-LINK_gdbserver`/`arm-none-eabi-gdb` shows the board running the cyclic executive continuously with zero hits on `_exit`/`abort`/`HardFault_Handler`/`FatalReceive_handler` breakpoints, and repeated `x/1xw 0x58021414` reads of `GPIOF_ODR` show bit 10 (`0x400`) toggling between set and clear across samples — confirming the LED blinker works end-to-end on the physical STM32H753XI-EVAL2 board with the full `ReferenceDeployment` topology active.

---

#### Hardware Bring-up: MicroFs Initialization and TaskRunner Dispatch-Table Corruption Fixes (September 2, 2026)
*Goal: Diagnose and resolve the two new FATAL asserts that surfaced only after uncommenting `taskRunner.runAll();` in `Main.cpp` — the step that begins actually dispatching every registered active component's message queue, rather than just letting the rate-group driver tick.*
*   [x] **Root-caused Bug #1 — `Os::Baremetal::MicroFs` never initialized:** A `bt full` on the first new FATAL showed `Svc::SystemResources::PhysMem()` (invoked periodically once `rateGroup_1Hz` began being dispatched) calling `Os::FileSystem::getFreeSpace("/")`, which hard-asserts (`FW_ASSERT(statePtr != nullptr)` in `fprime-baremetal/Os/Baremetal/FileSystem.cpp:123`) because `Os::Baremetal::MicroFs::MicroFsInit()` had never been called anywhere in the project — nothing in the framework calls it automatically.
*   [x] **Fixed by adding a `Os::Baremetal::MicroFs::MicroFsInit(config, id, allocator)` call** at the top of `configureTopology()` in `ReferenceDeployment/Top/ReferenceDeploymentTopology.cpp`, using a conservative static 2-bin config (2×1024-byte files + 1×4096-byte file) allocated from the existing 96 KiB bootstrap pool. Since the pool itself was already counted in `.bss`, this only added ~88 bytes of new static storage (for the `MicroFsConfig` struct), not the full file-data reservation.
*   [x] **Noted a pre-existing limitation (not yet fixed):** `MicroFs` only recognizes paths matching `/bin<N>/file<M>`; the literal paths used by `Svc::PrmDb`, `Svc::FileDownlink`/`FileUplink`, and `Svc::DpCatalog` (e.g. `PrmDb.dat`) will resolve to `Status::INVALID` rather than crash — real parameter/file persistence needs a follow-up path-mapping fix.
*   [x] **Root-caused Bug #2 — a real `fprime-baremetal` framework bug in `Os::Baremetal::TaskRunner::addTask()`:** After fixing Bug #1, a *different* new FATAL appeared: `Svc::TlmChanComponentBase::Run_handlerBase` (`TlmChanComponentAc.cpp:668`) asserting `qStatus == Os::Queue::OP_OK` with `qStatus = Os::QueueInterface::FULL` — `CdhCore::tlmSend`'s own internal dispatch queue was full because it was never being drained. Inspecting a GDB dump of `TaskRunner::m_task_table` showed only 18 entries with clear **duplicates** (`ComCcsds::aggregator`, `ComCcsds::comQueue`, `DataProducts::dpCat`, `FileHandling::fileUplink`, `ReferenceDeployment::cmdSeq`, and all three rate groups each appeared twice) while several components (`CdhCore::tlmSend`, `CdhCore::events`, `DataProducts::dpMgr`/`dpWriter`/`dpBufferAccumulator`, `FileHandling::fileManager`/`prmDb`) were **completely missing**.
*   [x] **Confirmed the root cause with a standalone simulation:** `TaskRunner::addTask()`'s "sort by priority during insertion" logic first wrote the new task directly into the tail slot (`m_task_table[m_index]`), then ran a *separate* swap-based bubble loop starting from index 0 with `sort_element` reinitialized to the same task — a fundamentally broken insertion-sort pattern that both duplicates and drops entries. A Python re-implementation of the exact algorithm, fed the project's real registration order and priorities (from `Priorities::*` in the generated topology header: `cmdDisp=35`, `events=23`, `tlmSend=22`, `aggregator=30`, `comQueue=29`, `dpBufferAccumulator=21`, `dpCat=24`, `dpMgr=23`, `dpWriter=22`, `fileDownlink=23`, `fileManager=22`, `fileUplink=24`, `prmDb=21`, `cmdSeq=40`, and the three rate groups at 41/42/43), reproduced the **exact same** corrupted table observed on hardware — proving conclusively that 7 of 17 active components (including `tlmSend`) were silently dropped from the round-robin dispatch table and therefore never had `doDispatch()` called, so any message queued to them (like the periodic `RUN_SCHED` the rate group sends `tlmSend`) simply accumulated until the queue overflowed.
*   [x] **Fixed `Os::Baremetal::TaskRunner::addTask()`** in `lib/fprime-baremetal/fprime-baremetal/Os/TaskRunner/TaskRunner.cpp`, replacing the broken swap logic with a correct O(n) shift-based insertion sort: find the first slot with a lower-priority occupant (or the first empty slot), shift every later entry one slot to the right, then place the new task. Re-ran the same Python simulation against the corrected algorithm and confirmed all 17 tasks are retained exactly once, none are dropped, and the table is properly sorted by descending priority.
*   [x] **Verified live on hardware:** Rebuilt, reflashed via `ST-LINK_gdbserver`/`arm-none-eabi-gdb`, and let the board free-run with `taskRunner.runAll()` fully enabled. Over 2+ minutes of continuous execution (checked via repeated `interrupt`/`bt`/`detach` cycles), the CPU was consistently found executing normal application code (`Svc::ActiveRateGroup::CycleIn_handler`, `Os::Stm32::RawTime` construction, etc.) with zero hits on `_exit`/`abort`/`HardFault_Handler`/`FatalReceive_handler` breakpoints, and repeated `x/1xw 0x58021414` reads of `GPIOF_ODR` confirmed bit 10 still toggling (`0x400` ↔ `0x0`) — confirming the full `ReferenceDeployment` topology, including every active component's cooperative dispatch, now runs cleanly end-to-end on the physical STM32H753XI-EVAL2 board.

---

#### Week 5: Ground Link (UART Driver with DMA)
*Goal: Implement non-blocking ground communications over USART1 using DMA to prevent the cyclic executive from stalling during serial transmission.*
*   [ ] **Implement `Drv::ByteStreamDriverModel`:** Create an F´ driver component for the STM32 USART1 peripheral.
*   [ ] **Configure Direct Memory Access (DMA):** Integrate DMA stream controllers for USART1 TX/RX [1]. This allows the Cortex-M7 to hand off the transmission of telemetry frames directly to the DMA controller, ensuring the CPU continues polling component queues without millisecond-level serial wait states.
*   [ ] **Wire Ground Interface:** Connect the UART driver output ports to the F´ downlink framer (`Svc::FprimeFramer`) and uplink deframer (`Svc::FprimeDeframer`).

---

#### Week 6: HAL Implementation & Platform Library Structure
*Goal: Establish a clean, isolated hardware translation layer for the STMicroelectronics libraries.*
 *   [x] **Structure `lib/fprime-stm32` Platform Library:** Create a reusable platform library (modeled after `fprime-vorago` or `fprime-arduino`) to house all low-level hardware abstraction logic.
 *   [x] **Integrate STM32Cube HAL/LL:** Import the required STM32CubeH7 HAL (Hardware Abstraction Layer) and LL (Low-Level) driver source files into your CMake platform library.
 *   [x] **Write the Flash Linker Script:** Adapt the memory mapping script (`STM32H753XIHx_FLASH.ld`) to define where code (.text), static variables (.data/.bss), and the stack are allocated within the 2MB Flash and 1MB SRAM.

---

#### Week 7: Hardware-Specific Device Driver Components
*Goal: Create standard F´ passive components that wrap STM32 GPIO and timer peripherals, isolating the flight logic from low-level register states.*
*   [ ] **Develop GpioDriver Component:** Create an F´ passive driver wrapping the STM32 GPIO register calls. It must implement the `gpioWrite` port type to handle output pin control.
*   [ ] **Develop Timer/Clock Tick Component:** Write a hardware timer driver component that handles an internal STM32 timer interrupt (e.g., TIM2) and raises periodic execution ticks.
*   [ ] **Apply Application-Manager-Driver Pattern:** Verify that all low-level hardware accesses are confined to these driver components, ensuring no application component touches hardware registers directly.

---

#### Week 8: LED Blinker "Spacecraft" Implementation
*Goal: Fully assemble your flight software topology, wire all components, and blink the physical LED via a periodic rate group.*
*   [ ] **Model Topology in FPP:** Define your component instances (`led`, `gpioDriver`, `rateGroup1`) inside `instances.fpp` and connect their input/output ports in `topology.fpp`.
*   [ ] **Connect Rate Group 1:** Wire the `RateGroupDriver` 1 Hz output to your application's passive `run` port, driving the blink cycle.
*   [ ] **Initialize Peripherals in Main:** Add callouts in your topology C++ initialization code (`LedBlinkerTopology.cpp`) to open **PF10** (User LED1) as an output via the GPIO driver.
*   [ ] **Establish GDS Ground Loop:** Launch your bare-metal executable on the board, boot up `fprime-gds`, and verify you can toggle the LED blink state via command and receive active telemetry.

---

#### Week 9: Automated Unit & HIL System Testing
*Goal: Validate the ported framework using automated test suites, capturing performance metrics.*
*   [ ] **Execute Unit Tests:** Use the **F´ Unit Test Framework** to write isolated component tests with simulated port histories [36, 37]. Run `fprime-util check` to ensure all tests pass locally.
*   [ ] **Develop HIL System Tests:** Write automated Python test scripts using the **GDS Integration Test API** (`pytest` and `fprime_test_api`).
*   [ ] **Test Real-Time Command & Telemetry:** Run the Python scripts to send `led.BLINKING_ON_OFF` commands to the physical board via USB. Use `assert_telemetry` to programmatically verify that the transition count (`LedTransitions`) increases correctly on the target hardware.
*   [ ] **Profile Runtime Performance:** Measure compile-time image size, SRAM utilization, and cyclic executive execution margins to prove the efficiency of the bare-metal port.

---

### Block 2: Advanced Device Drivers (Weeks 10-11)

#### Week 10: I2C driver architecture and polled/interrupt transfer baseline
*Goal: Add a reusable F´ passive I2C driver component to `lib/fprime-stm32`, with deterministic, bounded transaction handling and no application-level HAL access.*
*   [ ] **Enable and configure an STM32H7 I2C peripheral:** Select the board connector/pin routing for the first sensor bus, enable the required `HAL_I2C` source in the platform library, generate the matching CubeMX clock/GPIO/MSP/NVIC configuration, and document the selected bus, pins, pull-ups, and target bus rate (start at 400 kHz Fast-mode).
*   [ ] **Define the FPP component boundary:** Add a passive `Stm32.I2cDriver` model with typed request/response ports that map a sensor transaction into address, register/subaddress, read/write direction, `Fw::Buffer` payload, and completion status. The client supplies write data or a destination buffer through an input request port; the driver returns ownership and reports `OK`, `BUSY`, `NACK`, `TIMEOUT`, `ARBITRATION_LOST`, or `DMA_ERROR` through one completion output port.
*   [ ] **Implement a bounded single-flight state machine:** Permit exactly one in-flight I2C transaction; reject a second request with `BUSY` rather than allocating or queueing dynamically. Use a 10 ms transaction watchdog measured by the cyclic executive and recover a stuck bus by aborting the transaction, resetting the HAL state, and emitting a bounded F´ event.
*   [ ] **Keep ISR work minimal and interrupt-safe:** Route I2C event/error IRQ handlers only to the HAL IRQ entry points and have completion callbacks set volatile completion/error flags. Consume those flags, issue the F´ completion port, and return `Fw::Buffer` ownership exclusively from a deterministic `poll()`/scheduled context, not from an ISR.
*   [ ] **Add host-side component tests:** Test address/register encoding, one-request-at-a-time admission, all completion mappings, timeout recovery, and buffer return on every failure path with mocked low-level transfer functions. Build the STM32 target after the FPP model, implementation, and topology-facing API compile.
*   [ ] **Exit criterion:** On the board, run an I2C address probe or a known-register read 1,000 times at 400 kHz with zero leaked buffers, no main-loop blocking, and explicit telemetry/event evidence for each injected error path.

#### Week 11: SPI driver and DMA-safe transfer completion
*Goal: Deliver a reusable SPI passive component and establish the DMA/cache/ownership pattern shared by SPI and future high-rate buses.*
*   [ ] **Enable the selected STM32H7 SPI instance:** Configure SPI master mode, clock polarity/phase, chip-select GPIO, DMA request/stream, NVIC priority, and the HAL source files in `lib/fprime-stm32`; begin at 1 MHz and raise the clock only after scope or logic-analyzer verification of signal integrity.
*   [ ] **Define `Stm32.SpiDriver` ports:** Model a typed full-duplex transaction request containing chip-select index, transmit buffer, receive buffer, transfer length, and per-transfer mode/speed selection where hardware supports it. Provide completion/status and buffer-return ports so the caller retains explicit ownership of every DMA buffer.
*   [ ] **Use a fixed DMA-safe transfer contract:** Define 256-byte aligned AXI-SRAM TX and RX staging buffers for the initial driver, enforce a maximum transaction of 256 bytes, clean D-cache over TX ranges before starting DMA, invalidate D-cache over RX ranges after DMA completion, and reject buffers outside AXI/D2 SRAM until a validated address-range checker is available.
*   [ ] **Implement deterministic chip-select and recovery behavior:** Assert CS immediately before the transfer, deassert it exactly once on success, HAL error, or watchdog expiry, and abort/reset the SPI/DMA stream before reporting a failed transfer. Never spin waiting for `HAL_SPI_*_DMA()` completion.
*   [ ] **Integrate driver polling with the cyclic executive:** Call driver `poll()` after ISR completion flags are latched and before dependent application components are dispatched, preserving the repository’s manually dispatched queued-component pattern rather than introducing tasks.
*   [ ] **Exit criterion:** Complete 10,000 loopback or sensor WHO_AM_I transactions without corruption; capture transfer latency, maximum cyclic-executive iteration time, SPI error count, and AXI/D2 buffer addresses in the bring-up log.

### Block 3: Sensor Orchestration and `fprime-sensors` (Weeks 13-14)

#### Week 12: Controlled `fprime-sensors` integration and bus adaptation
*Goal: Bring one community sensor component into the build without coupling payload logic to STM32 HAL code.*
*   [ ] **Add the sensor library as a pinned submodule:** Add `fprime-community/fprime-sensors` under `lib/fprime-sensors`, record the exact commit SHA and compatible F´ version in `.gitmodules`/project documentation, register its library path in `settings.ini`, and ensure a fresh recursive clone can configure and build without untracked local dependencies.
*   [ ] **Select one physically available first sensor:** Choose an I2C IMU or temperature/pressure sensor that is supported upstream and wired to the selected evaluation-board/header bus. Record its I2C address or SPI chip select, voltage level, wiring, expected identity-register value, data rate, and unit conversion in the deployment documentation.
*   [ ] **Create an adapter only where port types differ:** Connect the community sensor’s I2C/SPI request and response ports directly to `Stm32.I2cDriver`/`Stm32.SpiDriver` when types match. Otherwise, add one thin passive F´ adapter that translates the community driver’s transaction type into the STM32 driver contract; do not embed HAL calls in the sensor component or topology implementation.
*   [ ] **Add sensor instances and static resource budgets:** Add the sensor and adapter instances to `ReferenceDeployment/Top/instances.fpp`, assign stable base IDs, use queue depth 4 or less for any queued sensor controller, and account for every static/DMA buffer in the linker map. Do not place I2C/SPI DMA payload buffers in DTCM.
*   [ ] **Establish initialization and fault reporting:** During `configureTopology()`, perform bounded sensor reset, identity check, configuration-register writes, and calibration/default configuration. Expose initialization failure, bus errors, and identity mismatch as F´ events and a health telemetry state rather than silently retrying forever.
*   [ ] **Exit criterion:** A cold boot identifies the physical sensor, applies its configuration, and reports a valid health state over the USART1 ground link without exceeding the established AXI SRAM budget.

#### Week 13: Scheduled sensor acquisition and science telemetry
*Goal: Produce calibrated physical measurements through F´ telemetry on a rate-group-driven schedule.*
*   [ ] **Add an acquisition state machine:** Implement explicit states such as `UNINITIALIZED`, `CONFIGURING`, `IDLE`, `READ_PENDING`, `CONVERTING`, and `FAULT`. A scheduled call begins a read only when `IDLE`; the completion callback advances state on the subsequent cyclic-executive pass.
*   [ ] **Schedule periodic reads deliberately:** Connect the sensor controller to the existing rate-group topology. Start at 10 Hz for IMU-class data or 1 Hz for temperature/pressure data, reserving a distinct rate-group member index and recording expected bus time, CPU budget, and maximum allowed missed samples.
*   [ ] **Publish raw and engineering telemetry:** Telemetrize raw register values, calibrated engineering units, sensor temperature, sample sequence counter, last-completion status, bus error count, and missed-sample count. Use fixed-size F´ types and generated telemetry channels; no heap allocation, vectors, or variable-length payload construction after initialization.
*   [ ] **Validate real measurements:** Compare 100 captured samples against a known physical reference or vendor tool; verify sign, scale, units, byte order, and timestamp cadence. Induce a disconnected-sensor/NACK condition and verify bounded recovery, event emission, continued cyclic execution, and no lost DMA-buffer ownership.
*   [ ] **Update dictionaries and ground artifacts:** Regenerate the F´ topology dictionary after telemetry changes and archive a representative decoded telemetry capture with the test evidence.
*   [ ] **Exit criterion:** The physical target streams stable sensor telemetry at its chosen rate for a 30-minute soak test, with no allocation after initialization, no DMA/cache fault, no queue overflow, and a documented error-recovery result.

### Block 4: Hardware-in-the-Loop Automated CI/CD (Weeks 15-16)

#### Week 14: Bench runner and reproducible flashing path
*Goal: Establish a dedicated, access-controlled local GitHub Actions runner connected to the STM32H753XI-EVAL2 for artifact deployment and serial observation.*
*   [ ] **Provision the HIL host:** Use a dedicated Linux host with a self-hosted GitHub Actions runner, ARM GNU toolchain, F´ Python environment, `openocd`, ST-LINK utilities, `pyserial`, and Python test dependencies. Connect the board’s CN23 ST-LINK-V3E USB interface for SWD and virtual COM port, plus stable board power through CN10.
*   [ ] **Assign a hardware-runner label and concurrency lock:** Create a uniquely labeled runner such as `self-hosted`, `linux`, and `stm32h753-hil`; configure the workflow so exactly one job owns the physical board at a time and prevent untrusted fork pull requests from accessing it.
*   [ ] **Create a fail-fast flash/reset script:** Build `ReferenceDeployment` for `stm32h7`, verify the ELF exists and has a valid vector table, program it using either `openocd` with `program <elf> verify reset exit` or `st-flash --reset write`, and fail immediately on probe, erase, program, verify, or reset errors. Capture the tool version, board serial number, ELF SHA-256, and console log as CI artifacts.
*   [ ] **Add a reproducible board reset and serial-discovery step:** Identify the CN23 VCP by stable `/dev/serial/by-id/` path, reset/drain the port before each test, and enforce a 10-second boot banner/heartbeat timeout. Do not depend on transient `/dev/ttyACM*` numbering.
*   [ ] **Gate the workflow in stages:** Run host formatting/unit checks first, cross-build second, and hardware flash only after those pass. Initially trigger HIL on manual dispatch and protected-branch pushes; enable pull-request execution only after runner isolation and repository trust boundaries are reviewed.
*   [ ] **Exit criterion:** Ten consecutive clean CI runs build, flash, reset, and acquire the expected board boot indication without human unplug/replug intervention.

#### Week 15: UART-DMA command/telemetry HIL tests
*Goal: Turn the HIL runner into an automated verification system for the actual F´ ground protocol and the physical serial path.*
*   [ ] **Implement a pytest hardware fixture:** Open the stable ST-LINK VCP at the deployment baud rate, flush stale input, acquire an exclusive test lock, flash/reset the target, wait for a bounded ready indication, and guarantee fixture cleanup/reset on test failure.
*   [ ] **Use the F´ framing protocol end-to-end:** Send a correctly framed `NO_OP` command through the host serial port to USART1 RX DMA, wait for the F´ command-response event/status through USART1 TX DMA, and decode it using the generated deployment dictionary or a deliberately versioned test decoder.
*   [ ] **Assert protocol-level behavior, not console strings:** Verify command opcode/sequence correlation, dispatch success, `COMMAND_OK` completion status, response latency below an agreed initial threshold such as 500 ms, and a telemetry/event counter proving the request traversed DMA, deframing, routing, command dispatch, and the reply path.
*   [ ] **Add negative-path tests:** Send a malformed frame, bad CRC/length where enabled, unsupported opcode, and a command during DMA busy state. Assert a bounded reject/error response, no hard fault/reset, and continued success of a subsequent `NO_OP`.
*   [ ] **Collect runtime margin evidence:** Export build size, `.bss`, `.dtcm_bss`, DMA buffer locations, maximum observed command round-trip time, DMA completion/error counters, and test logs as CI artifacts. Fail the job if AXI SRAM/DTCM limits or the defined command response timeout are exceeded.
*   [ ] **Exit criterion:** The protected-branch HIL workflow reliably passes a valid `NO_OP`, error-path tests, and UART-DMA health checks on the physical STM32 target with archived evidence for each run.

### Block 5: YAMCS Mission Control Ground System (Weeks 16-17)

#### Week 16: F´-to-YAMCS packet and command definition bridge
*Goal: Define a versioned, testable ground-data interface instead of treating F´ dictionaries as directly consumable YAMCS configuration.*
*   [ ] **Freeze the on-wire packet contract:** Document the exact bytes emitted by the USART1 link: F´ framing/deframing layer, packet identifiers, length fields, endianness, command sequence/response correlation, checksum/CRC behavior, and any CCSDS encapsulation. Select one stable telemetry packet format before creating YAMCS definitions.
*   [ ] **Generate and transform F´ metadata:** Regenerate the deployment topology dictionary and command/telemetry metadata after every model change. Write a versioned conversion tool that consumes those generated JSON/XML artifacts and produces reviewed YAMCS XTCE/mission-database definitions; do not hand-copy channel IDs or opcodes into multiple files.
*   [ ] **Define YAMCS commands and telemetry parameters:** Map `NO_OP`, LED control, LED transition count, sensor engineering values, sequence/status fields, and error counters to YAMCS command containers and telemetry parameters. Preserve F´ IDs, serialized field widths, units, calibration, valid ranges, and packet offsets in generated or source-controlled definitions.
*   [ ] **Create decoder conformance tests:** For each supported command/packet, compare a golden binary frame created by the F´ serializer or HIL capture with the YAMCS decoder result. Make dictionary/packet-layout drift fail in CI before a ground-system release.
*   [ ] **Exit criterion:** A recorded target telemetry frame decodes in YAMCS with correct names, types, units, timestamps, and values, and a YAMCS-issued `NO_OP` encodes to a byte-identical validated F´ uplink frame.

#### Week 17: YAMCS deployment, serial bridge, and operations dashboard
*Goal: Demonstrate a mission-control-style command and telemetry loop from YAMCS to the physical board and back.*
*   [ ] **Deploy an isolated YAMCS server on the HIL host:** Use a pinned YAMCS version and a source-controlled mission configuration. Configure an authenticated local operator account, persistent archive location, packet link, command link, alarms, and startup procedure; do not expose the board serial device or unauthenticated command endpoint on a public network.
*   [ ] **Connect YAMCS to the physical transport:** Configure a serial packet link directly to the stable CN23 VCP, or use a narrowly scoped serial-to-TCP bridge when YAMCS requires TCP. The bridge must forward the binary F´ frames intact, enforce a single writer, reconnect predictably, and report link-up/down state without consuming or rewriting DMA payload bytes.
*   [ ] **Validate command authority and acknowledgments:** Issue `NO_OP` and LED-control commands from the YAMCS command stack, correlate F´ command responses back to YAMCS acknowledgments, and reject stale/unknown responses by sequence number. Verify behavior through both normal operation and disconnect/reconnect testing.
*   [ ] **Build the initial operations dashboard:** Add real-time widgets for LED transition count, LED state, sensor engineering measurements, sample sequence, sensor health, I2C/SPI error count, USART1 DMA error count, and command success/failure. Add conservative warning alarms for stale sensor data, repeated bus failures, UART-DMA errors, and missing telemetry.
*   [ ] **Run an end-to-end demonstration and handover:** Perform a 30-minute supervised run in which YAMCS commands the physical LED, displays live sensor data, archives telemetry, and recovers from one controlled serial disconnect. Publish the operator quick-start, port map, packet-interface version, dashboard screenshots, test results, and follow-on issue list.
*   [ ] **Exit criterion:** A new developer can provision the documented YAMCS configuration, connect to the physical board, observe live LED/sensor telemetry, command `NO_OP` and LED state changes, and retrieve archived command/telemetry evidence without modifying flight code.

### Block 6: Final Presentation

#### Week 18: Final Handover, Presentation & Community Upstream
*Goal: Document the bare-metal architecture and present your findings to the JPL flight software group.*
*   [ ] **Document Porting Architecture:** Write detailed Markdown guides explaining your platform tailoring, OSAL overrides, and DMA driver designs for the repository's `/docs` directory.
*   [ ] **Prepare Upstream Pull Requests:** Clean your code to align with JPL coding standards and submit pull requests to the upstream `fprime-baremetal` community repository to share your platform abstractions.
*   [ ] **Final JPL Presentation:** Present the completed **STM32H7 Bare-Metal F´ Reference Project** to the **Flight Software Architecture and Infrastructure Group**, demonstrating automated system execution, test coverage, and memory profiling.

---