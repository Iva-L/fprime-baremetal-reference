# F´ STM32H7 Platform Support

This repository contains the STM32H7 hardware, OSAL, and driver support used by the
bare-metal F´ reference deployment. It targets the STM32H753XI-EVAL2 evaluation
board and is intended to be reusable as a Git submodule.

The implementation is designed for a bare-metal cyclic executive:

- No FreeRTOS or other operating system is required.
- `Os::Task` is cooperative and creates no thread.
- `Os::Mutex` provides bounded critical sections by preserving and restoring
  Cortex-M7 `PRIMASK`.
- `Os::Queue` is a fixed-depth, interrupt-safe FIFO allocated during startup.
- `Os::RawTime` provides a race-safe microsecond clock from a free-running TIM2
  counter and overflow interrupt.
- DMA buffers are placed in DMA-accessible AXI SRAM rather than DTCM.

## Supported hardware

The current configuration is for the STM32H753XIH6 on the
STM32H753XI-EVAL2:

- Cortex-M7, 2 MiB Flash, 1 MiB RAM
- 25 MHz HSE input and a 480 MHz PLL1 system clock
- USART1 through the embedded ST-LINK-V3E VCP
- USART1 TX on PB14 and RX on PB15
- Direct user LEDs on PF10 (LED1, green) and PA4 (LED3, red)
- TIM2 as the 1 MHz, 32-bit free-running time source

LED2 and LED4 are routed through the external MFX I2C expander and are not
used by the direct bare-metal GPIO examples.

## Contents

- `Drivers/CMSIS`: STM32H7 CMSIS device headers and startup support
- `Drivers/STM32H7xx_HAL_Driver`: the selected STM32 HAL implementation
- `Os`: STM32H7 bare-metal delegates for Task, Mutex, Queue, and RawTime
- `Drv/STM32GpioDriver`: passive GPIO input/output driver
- `Drv/STM32Timer`: TIM2 channel 2 output-compare tick driver
- `Drv/STM32UartDriver`: USART1 DMA-backed byte-stream driver
- `include`: shared HAL configuration, cache helpers, and interrupt declarations
- `src`: clock, MSP, peripheral, interrupt, and TIM2 clock support

`CMakeLists.txt` builds the HAL support as `FprimeStm32` and registers the OSAL
and driver subdirectories with F´. The real interrupt implementation in
`src/stm32h7xx_it.c` must be linked directly into the deployment executable,
rather than only through the static library, so its strong handlers override the
startup file's weak `Default_Handler` aliases.

## Integration and build

The parent F´ project selects this platform through its `stm32h7` CMake
configuration. From the parent project:

```shell
source fprime-venv/bin/activate
fprime-util generate -f
fprime-util build -j"$(nproc)"
```

To build the reference deployment directly after generation:

```shell
ninja -C build-fprime-stm32h7 ReferenceDeployment
```

The deployment must call the hardware initialization routines in this order:

1. `HAL_Init()`
2. `FprimeStm32_ClockInit()`
3. `SCB_EnableICache()` and `SCB_EnableDCache()`
4. `Stm32_Tim2ClockInit()`
5. topology setup and the cyclic executive

The clock initialization is required before configuring TIM2. The cache
initialization is required before using the DMA cache-maintenance helpers;
Cortex-M7 cache tags and data are undefined at reset.

## USART1 DMA ground link

`Drv::Stm32UartDriver` provides a non-blocking USART1 DMA transport over PB14/PB15
through the ST-LINK-V3E VCP. It uses fixed-size TX and RX rings, aligned DMA
staging buffers, idle-line detection for RX, and a polled DMA state machine so
the cyclic executive is never blocked waiting for serial I/O.

The driver was validated on the physical STM32H753XI-EVAL2 with:

- 115200 baud, 8N1 operation
- DMA on both transmit and receive
- 183 KiB transmitted in 180 seconds with zero TX/RX error counts
- 6,144 bytes captured from the VCP in 6.0 seconds, matching the driver's
  internal byte counter
- 96 injected uplink bytes received and drained without buffer leaks
- CCSDS Space Packet framing through the `ComCcsds` subtopology
- A continuous 21+ minute `fprime-gds` session with command uplink, telemetry
  downlink, and event downlink

The GDS validation decoded more than 12,000 telemetry samples at approximately
10 samples per second. The command round trip included `CMD_NO_OP`, string
commands, and an oversized string that correctly returned `FORMAT_ERROR`.

The ground-link tests above were performed before the September 3 clock-tree
fix, while the board was still running from the approximately 64 MHz HSI. The
USART baud rate self-adjusted from the live peripheral clock query, so the link
remained valid, but an extended ground-link soak at the corrected PLL clock is
still required.

## Hardware validation

The complete reference topology has been run on the physical board with the
non-blocking cyclic executive and cooperative dispatch enabled. Validation
included:

- Topology setup and all active-component queues created successfully
- No hits on assertion, abort, exit, HardFault, BusFault, or fatal-handler
  breakpoints during the recorded endurance runs
- PF10 LED activity confirmed through live GDB reads of `GPIOF_ODR`
- TIM2 measured at approximately 997.9 kHz over an undisturbed 30-second
  interval after PLL clock initialization
- The 100 Hz timer tick and rate-group tick counters remained synchronized
- USART1 DMA continued transmitting correctly after the corrected clock was
  enabled

The clock-tree bug was fixed by calling `FprimeStm32_ClockInit()` after
`HAL_Init()`. Before that change, TIM2 advanced at approximately 269.5 kHz,
which matched the unconfigured 64 MHz HSI divided by the intended TIM2
prescaler. The corrected implementation selects PLL1 and restores the intended
480 MHz system clock.

## Memory and placement

The STM32 linker configuration defines these regions:

| Region | Capacity |
| --- | ---: |
| `FLASH` | 2 MiB |
| `AXI_SRAM` | 512 KiB |
| `DTCM_RAM` | 128 KiB |

CPU-only framework state can be placed in DTCM, while DMA-visible buffers and
the bootstrap allocation pool remain in AXI SRAM. The latest recorded
`baremetal-size stm32h7` result for the complete reference deployment was:

| Region | Used | Remaining |
| --- | ---: | ---: |
| Flash | 560,160 bytes | 73.3% |
| AXI SRAM `.bss` | 226,988 bytes | 56.7% |
| DTCM `.dtcm_bss` | 8,584 bytes | 93.5% |
| Bootstrap pool | 112,656 of 131,072 bytes | 14.1% |

The reference deployment locks the bootstrap allocator after topology setup and
wraps the C heap symbols so post-initialization allocations assert instead of
silently using an unbounded heap. New components should be evaluated against
both the AXI SRAM margin and the remaining bootstrap-pool capacity.

## Known follow-up work

- Repeat the extended USART1 DMA and GDS soak at the corrected 480 MHz clock.
- Validate TIM2 rollover, interrupt masking, and long-duration stability.
- Connect `Drv::Stm32GpioDriver` to a deployment topology.
- Continue hardware-in-the-loop automation for the STM32 target.
- Add real persistent file support for the MicroFs-backed services; the current
  conservative configuration recognizes only `/bin<N>/file<M>` paths.
