# Stm32::Stm32UartDriver

## 1. Introduction

`Stm32UartDriver` is the STM32H753 ground-link driver for USART1 (PB14 TX / PB15 RX, routed to the STLINK-V3E virtual COM port on CN23). It implements the [`Drv.ByteStreamDriver`](../../../fprime/Drv/Interfaces/ByteStreamDriver.fpp) interface using DMA-backed, non-blocking transfers so the bare-metal cyclic executive never stalls waiting on serial I/O.

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

## 4. Usage

```cpp
// configureTopology(), after commsBufferManager sizing is known:
const Fw::Success comDriverOpened = comDriver.open(FW_COM_BUFFER_MAX_SIZE, USART1_IRQn, USART1_IRQ_PREEMPT_PRIORITY, USART1_IRQ_SUB_PRIORITY, BAUD_RATE);
    if(comDriverOpened == Fw::Success::FAILURE) {
        Fw::Logger::log("[ERROR] Failed to open UART\n");
    }

// Main.cpp cyclic executive loop, every pass:
static_cast<Stm32::Stm32UartDriverComponentBase&>(ReferenceDeployment::comDriver).run_handlerBase(0, 0);
```

## 5. Events and telemetry

Events: `PortOpened`, `HalError`, `UartError`, `TxRingFull`, `RxRingFull`, `TxTimeout`, `NoBuffers`. Telemetry: `BytesSent`, `BytesRecv`, `TxErrorCount`, `RxErrorCount`.