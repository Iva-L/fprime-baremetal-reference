# Stm32::Stm32UartDriver

## 1. Introduction

`Stm32UartDriver` is a DMA-backed, non-blocking `Drv.ByteStreamDriver` for any STM32H7 USART/UART peripheral — which instance a given component instance owns is chosen at `open()` time via the `Stm32::UsartInstance` enum, not hardcoded to one board's wiring. On this reference deployment it is opened against **USART1** (PB14 TX / PB15 RX, routed to the STLINK-V3E virtual COM port on CN23), but the driver itself has no USART1-specific code path; see section 3.6 "Choosing a USART instance" for what a different board/project needs to add to use a different one. It implements the [`Drv.ByteStreamDriver`](../../../fprime/Drv/Interfaces/ByteStreamDriver.fpp) interface using DMA-backed, non-blocking transfers so the bare-metal cyclic executive never stalls waiting on serial I/O.

Unlike [`Drv::LinuxUartDriver`](../../../fprime/Drv/LinuxUartDriver/docs/sdd.md), which this component started from, there is no receive thread: the bare-metal execution model forbids RTOS threads, so every DMA start/completion decision, error recovery step, and `Fw::Buffer` ownership transfer happens from a single polled state machine (`run_handler`), called directly from the cyclic executive every loop pass (see `Main.cpp`) rather than through a rate group, since a 1 Hz/0.5 Hz/0.25 Hz cadence would be far too slow for a byte-stream driver.

## 2. Requirements

| Name | Description | Validation |
|---|---|---|
| STM32-UART-COMP-001 | Shall implement the `Drv.ByteStreamDriver` interface | inspection |
| STM32-UART-COMP-002 | Shall never block the cyclic executive on USART1 TX or RX | inspection |
| STM32-UART-COMP-003 | Shall use DMA for both TX and RX, with idle-line detection on RX | inspection |
| STM32-UART-COMP-004 | Shall keep all DMA-visible buffers in AXI SRAM, not DTCM | inspection |
| STM32-UART-COMP-005 | Shall perform D-cache clean/invalidate around every DMA transfer | inspection |
| STM32-UART-COMP-006 | Shall recover from UART/DMA errors and transaction timeouts without hanging | test |
| STM32-UART-COMP-007 | Shall report telemetry for bytes sent/received and TX/RX error counts | inspection |
| STM32-UART-COMP-008 | Shall never perform F´ port calls or allocation from ISR context | inspection |

## 3. Design

### 3.1 Port model

Same as any `Drv.ByteStreamDriver` implementation: `allocate`/`deallocate` (buffer management), `send` (guarded, synchronous), `recv`/`ready` (outputs), `recvReturnIn` (guarded). `run` (`Svc.Sched`) is intentionally left unconnected in the topology; it is invoked directly via `run_handlerBase()` from `Main.cpp` (built with `FW_DIRECT_PORT_CALLS`), the same pattern used for `RateGroupDriver::CycleIn_handlerBase()`.

### 3.2 TX path

`send_handler()` copies the caller's `Fw::Buffer` into a 4096-byte AXI SRAM ring buffer (`m_txRing`) and returns immediately — `OP_OK` if the whole buffer fit, `SEND_RETRY` (with a throttled `TxRingFull` event) if it didn't. No partial enqueue: either all of the caller's data is copied, or none of it is, so the caller's ownership-retention contract for `send` holds cleanly.

`run_handler()`'s `pollTx()` step drains the ring into a 1024-byte, 32-byte-aligned staging buffer (`m_txStaging`), cleans the D-cache over that range (`Stm32::CleanDCacheForDma`), and calls `HAL_UART_Transmit_DMA()`. `HAL_UART_TxCpltCallback()` (ISR) only clears a `volatile` busy flag; `pollTx()` uses `Os::RawTime` to enforce a 10 ms watchdog and recovers (`HAL_UART_AbortTransmit`, `TxTimeout` event, `TxErrorCount` telemetry) if a transfer never completes.

### 3.3 RX path

USART1 is kept continuously armed with `HAL_UARTEx_ReceiveToIdle_DMA()` into a 1024-byte, 32-byte-aligned staging buffer (`m_rxStaging`). `HAL_UARTEx_RxEventCallback()` (ISR) only latches the received length and a `volatile` ready flag — it never touches F´ ports or the cache.

`run_handler()`'s `pollRx()` step, when that flag is set: invalidates the D-cache over the staging range, copies the bytes into a 4096-byte AXI SRAM ring buffer (`m_rxRing`, emitting a throttled `RxRingFull` event and counting an `RxErrorCount` if the ring has no room), and immediately re-arms `ReceiveToIdle_DMA` so no bytes are lost while the rest of the poll runs. It then drains whatever is queued in `m_rxRing` into one `allocate_out()`-obtained `Fw::Buffer` and forwards it via `recv_out()` with `OP_OK`; frame boundaries are the concern of the downstream `Svc::FrameAccumulator`/deframer, not this driver. If no buffer is available, a throttled `NoBuffers` event fires and the bytes stay queued for the next poll.

### 3.4 Error recovery

`HAL_UART_ErrorCallback()` (ISR) only latches `huart1.ErrorCode` and a pending flag. `run_handler()`'s `recoverUartError()` step aborts any in-flight TX/RX DMA transfer, clears the HAL error state, increments `TxErrorCount`/`RxErrorCount` based on the specific error bits (ORE/FE/PE/NE vs. DMA error), emits `UartError`, and re-arms RX reception — the driver never silently retries forever or leaves USART1/DMA in a stuck state.

### 3.5 Cache maintenance

`lib/fprime-stm32/include/CacheMaintenance.hpp` wraps CMSIS `SCB_CleanDCache_by_Addr`/`SCB_InvalidateDCache_by_Addr`. Both staging buffers are declared `alignas(32)` and sized as multiples of 32 bytes so cache-line-granular maintenance never touches unrelated memory.

### 3.6 Choosing a USART instance

The header (`Stm32UartDriver.hpp`) declares a HAL-free `enum class UsartInstance { Usart1, Usart2, Usart3, Uart4, Uart5, Usart6, Uart7, Uart8 }` covering every USART/UART peripheral an STM32H753 has — no `USART_TypeDef*`/`IRQn_Type` in the header, same convention as `Stm32GpioDriver`'s `GpioPort`. `open()` takes a `UsartInstance` instead of a raw NVIC IRQ number, so a caller cannot pass a mismatched instance/IRQn pair — the IRQn is derived internally.

All of the instance-specific mapping lives in `Stm32UartDriver.cpp` (the real HAL boundary, never in `Stm32UartDriverCommon.cpp`):

- `toIrqn(UsartInstance)` is a complete switch over all 8 instances — this mapping is fixed by the chip's vector table, so it doesn't depend on what a given project has actually wired up.
- `toHalHandle(UsartInstance)` (maps to the real `UART_HandleTypeDef*`, e.g. `&huart1`) and `callInstanceInit(UsartInstance)` (calls the matching `MX_USARTn_UART_Init()`) are only implemented for the instances this project's own CubeMX-generated `lib/fprime-stm32/src/usart.c` actually declares — currently just `USART1`/`huart1`/`MX_USART1_UART_Init()`. Any other case hits `FW_ASSERT` rather than silently doing nothing.
- The resolved handle is cached in a file-scope `s_huart` pointer (alongside the existing single-instance callback trampoline `s_instance` — see 3.7) so `hwStartTx()`/`hwAbortTx()`/`hwRestartRx()`/`hwAbortRx()`/`hwClearUartError()` never reference `huart1` by name.
- The HAL callbacks (`HAL_UART_TxCpltCallback`/etc.) match by pointer identity (`huart == s_huart`) instead of `huart->Instance == USART1`, so they work for whichever instance was opened without needing to know which one that is.

**Using a different instance on a different board:** generate that instance's CubeMX plumbing in your own project (a `huart2`/`MX_USART2_UART_Init()` in your `usart.c`, its DMA streams in `dma.c`, and a `USART2_IRQHandler` in your `stm32h7xx_it.c` calling `HAL_UART_IRQHandler(&huart2)` — the vectored IRQ handler name is fixed per instance by the chip, so this one step cannot be abstracted away), add the matching case to `toHalHandle()`/`callInstanceInit()`, and pass `Stm32::UsartInstance::Usart2` to `open()`. Nothing else in the driver changes.

### 3.7 Single-instance callback trampoline

Like `Stm32::STM32Timer` (see its own `docs/sdd.md` 2), the real HAL callbacks are plain C free functions with no user-context pointer, so `hwOpen()` stashes `this` into a file-scope `s_instance` the first time it runs. This means at most one `Stm32UartDriver` instance may be open **per physical USART/UART peripheral** at a time: opening a second driver instance against the *same* instance would silently overwrite `s_instance`/`s_huart` and steal the first instance's callbacks. Opening two driver instances against two *different* instances (e.g. one for USART1, one for USART2) is not supported by this trampoline as written today, since it tracks only one `(s_instance, s_huart)` pair project-wide — extending it to N pairs (one per instance actually wired up) would be a small, mechanical follow-up if a project ever needs more than one USART driven through this component.

## 4. Usage

```cpp
// configureTopology(), after commsBufferManager sizing is known:
const Fw::Success comDriverOpened = comDriver.open(FW_COM_BUFFER_MAX_SIZE, Stm32::UsartInstance::Usart1, USART_IRQ_PREEMPT_PRIORITY, USART_IRQ_SUB_PRIORITY, BAUD_RATE);
    if(comDriverOpened == Fw::Success::FAILURE) {
        Fw::Logger::log("[ERROR] Failed to open UART\n");
    }

// Main.cpp cyclic executive loop, every pass:
static_cast<Stm32::Stm32UartDriverComponentBase&>(ReferenceDeployment::comDriver).run_handlerBase(0, 0);
```

To use a different UART/USART instance: enable it in `Stm32Config.hpp` (its `USARTn_Instance` macro), regenerate the CubeMX project with that peripheral configured, call `open()`, then pass the matching `Stm32::I2cInstance` value to `open()`.

Events: `PortOpened`, `HalError`, `UartError`, `TxRingFull`, `RxRingFull`, `TxTimeout`, `NoBuffers`. Telemetry: `BytesSent`, `BytesRecv`, `TxErrorCount`, `RxErrorCount`.