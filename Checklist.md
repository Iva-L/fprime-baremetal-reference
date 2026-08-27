#  F Prime Baremetal Reference Project
*Target Hardware:* **STM32H753XI-EVAL2** (ARM Cortex-M7, 2MB Flash, 1MB SRAM)
*Execution Model:* **Pure Bare-Metal Cyclic Executive (Single-Threaded Polling Loop)**
*Primary Mentors:* Kevin Ortega (Mentor), Jeff Levison (Group Lead)

## Current Status Snapshot
*Current phase:* **Memory and framework tailoring before OSAL implementation.**
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
*   [x]  Tailor the framework and memory map before implementing OSAL primitives.
*   [x] Board startup and linker script for STM32H753 memory layout.

*What remains:*

*   [ ] Cyclic-executive `main()` dispatch loop and hardware init.
*   [ ] GPIO/UART configuration for LED1/LED3 and USART1 PB14/PB15.
*   [ ] DMA-backed USART1 ground communication using the byte-stream model.
*   [ ] Full application topology and hardware-aware component integration.

## Week-by-Week Schedule & Task Checklist

### Week 1: Board Bring-up & IT Onboarding
*Goal: Initialize host development environments, complete mandatory security training, and verify basic hardware operation.*
*   [x] **JPL IT Onboarding & Training:** Secure IT asset access, set up your JPL laptop, and complete mandatory IT/security training.
*   [x] **Local WSL Ubuntu Environment Setup:** Install the required dependencies for F´ on your local WSL Ubuntu partition.
*   [x] **Framework Verification:** Build and run the F´ **Hello World** and local **LED Blinker** tutorials on your host machine to confirm toolchain functionality.
*   [x] **Hardware Verification (Bring-up):** Connect your STM32H753XI-EVAL2 board to your host via USB Micro-B to the **CN23 STLINK-V3E port**. Verify board communication, update the debugger firmware, and verify jumper settings.
*   [x] **Baremetal Blinky Test:** Compile and flash a simple bare-metal LED-blinking program using STM32CubeIDE to verify that the target board is fully functional.

---

### Week 2: Bare-metal Execution Model (Cyclic Executive) Learning
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

### Week 3: Framework Tailoring & Memory Configuration
*Goal: Establish the memory contract and reduce the F´ deployment to a viable STM32H753 image before OSAL work.*
*   [x] **Reduce ReferenceDeployment footprint:** Remove or defer host-oriented subtopologies and unused services.
*   [x] **Tune `FpConfig.h`:** Reduce queue, stack, telemetry, and buffer defaults for the bare-metal target.
*   [x] **Linker Script SegmentationMap linker sections:** Distribute `.data` and `.bss` across DTCM, AXI SRAM, D2 SRAM, and D3 SRAM deliberately.
*   [x] **Linker Script Segmentation:** Distribute static allocations so that non-DMA variables are routed to DTCM, leaving AXI SRAM open for DMA communication buffers.
*   [x] **Section Zero-Initialization:** Clear `_sdtcm_bss` through `_edtcm_bss` in `Reset_Handler` before C++ constructors run.
*   [x] **Enforce the Zero Dynamic Memory Contract:** Route C++ `new` through a fixed bootstrap pool and lock allocation before cyclic execution.
*   [ ] **Profiling & Sizing Validation:** Integrate Kevins's profiling tools to gain exact visibility into which components consume memory.
*   [ ] **Topology Cleanup:** Integrate Kevins's profiling tools to gain exact visibility into which components consume memory.
### Week 4: Bare-metal OS Abstraction Layer (OSAL) Primitives
*Goal: Implement OSAL behavior against the established memory and execution contract.*
*   [ ] **Deconstruct `Os` Library:** Map the standard `Os` namespace interface to the bare-metal implementation.
*   [ ] **Implement Polling Task OSAL:** Map `Os::Task` to bounded execution blocks in the cyclic executive.
*   [ ] **Implement Interrupt-Safe Mutexes:** Use Cortex-M7 critical sections where required.
*   [ ] **Configure Custom `Os::Time` Implementation:** Connect the hardware timer to F´ time services.

---

### Week 5: Ground Link (UART Driver with DMA)
*Goal: Implement non-blocking ground communications over USART1 using DMA to prevent the cyclic executive from stalling during serial transmission.*
*   [ ] **Implement `Drv::ByteStreamDriverModel`:** Create an F´ driver component for the STM32 USART1 peripheral.
*   [ ] **Configure Direct Memory Access (DMA):** Integrate DMA stream controllers for USART1 TX/RX [1]. This allows the Cortex-M7 to hand off the transmission of telemetry frames directly to the DMA controller, ensuring the CPU continues polling component queues without millisecond-level serial wait states.
*   [ ] **Wire Ground Interface:** Connect the UART driver output ports to the F´ downlink framer (`Svc::FprimeFramer`) and uplink deframer (`Svc::FprimeDeframer`).

---

### Week 6: HAL Implementation & Platform Library Structure
*Goal: Establish a clean, isolated hardware translation layer for the STMicroelectronics libraries.*
 *   [x] **Structure `lib/fprime-stm32` Platform Library:** Create a reusable platform library (modeled after `fprime-vorago` or `fprime-arduino`) to house all low-level hardware abstraction logic.
 *   [x] **Integrate STM32Cube HAL/LL:** Import the required STM32CubeH7 HAL (Hardware Abstraction Layer) and LL (Low-Level) driver source files into your CMake platform library.
 *   [x] **Write the Flash Linker Script:** Adapt the memory mapping script (`STM32H753XIHx_FLASH.ld`) to define where code (.text), static variables (.data/.bss), and the stack are allocated within the 2MB Flash and 1MB SRAM.

---

### Week 7: Hardware-Specific Device Driver Components
*Goal: Create standard F´ passive components that wrap STM32 GPIO and timer peripherals, isolating the flight logic from low-level register states.*
*   [ ] **Develop GpioDriver Component:** Create an F´ passive driver wrapping the STM32 GPIO register calls. It must implement the `gpioWrite` port type to handle output pin control.
*   [ ] **Develop Timer/Clock Tick Component:** Write a hardware timer driver component that handles an internal STM32 timer interrupt (e.g., TIM2) and raises periodic execution ticks.
*   [ ] **Apply Application-Manager-Driver Pattern:** Verify that all low-level hardware accesses are confined to these driver components, ensuring no application component touches hardware registers directly.

---

### Week 8: LED Blinker "Spacecraft" Implementation
*Goal: Fully assemble your flight software topology, wire all components, and blink the physical LED via a periodic rate group.*
*   [ ] **Model Topology in FPP:** Define your component instances (`led`, `gpioDriver`, `rateGroup1`) inside `instances.fpp` and connect their input/output ports in `topology.fpp`.
*   [ ] **Connect Rate Group 1:** Wire the `RateGroupDriver` 1 Hz output to your application's passive `run` port, driving the blink cycle.
*   [ ] **Initialize Peripherals in Main:** Add callouts in your topology C++ initialization code (`LedBlinkerTopology.cpp`) to open **PF10** (User LED1) as an output via the GPIO driver.
*   [ ] **Establish GDS Ground Loop:** Launch your bare-metal executable on the board, boot up `fprime-gds`, and verify you can toggle the LED blink state via command and receive active telemetry.

---

### Week 9: Automated Unit & HIL System Testing
*Goal: Validate the ported framework using automated test suites, capturing performance metrics.*
*   [ ] **Execute Unit Tests:** Use the **F´ Unit Test Framework** to write isolated component tests with simulated port histories [36, 37]. Run `fprime-util check` to ensure all tests pass locally.
*   [ ] **Develop HIL System Tests:** Write automated Python test scripts using the **GDS Integration Test API** (`pytest` and `fprime_test_api`).
*   [ ] **Test Real-Time Command & Telemetry:** Run the Python scripts to send `led.BLINKING_ON_OFF` commands to the physical board via USB. Use `assert_telemetry` to programmatically verify that the transition count (`LedTransitions`) increases correctly on the target hardware.
*   [ ] **Profile Runtime Performance:** Measure compile-time image size, SRAM utilization, and cyclic executive execution margins to prove the efficiency of the bare-metal port.

---

### Week 10: Final Handover, Presentation & Community Upstream
*Goal: Document the bare-metal architecture and present your findings to the JPL flight software group.*
*   [ ] **Document Porting Architecture:** Write detailed Markdown guides explaining your platform tailoring, OSAL overrides, and DMA driver designs for the repository's `/docs` directory.
*   [ ] **Prepare Upstream Pull Requests:** Clean your code to align with JPL coding standards and submit pull requests to the upstream `fprime-baremetal` community repository to share your platform abstractions.
*   [ ] **Final JPL Presentation:** Present the completed **STM32H7 Bare-Metal F´ Reference Project** to the **Flight Software Architecture and Infrastructure Group**, demonstrating automated system execution, test coverage, and memory profiling.