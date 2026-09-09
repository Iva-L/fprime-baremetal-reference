# Stm32::STM32Timer

## 1. Introduction

`STM32Timer` is the periodic hardware tick source that drives the bare-metal cyclic executive's rate groups. It implements the [`Drv.Tick`](../../../../fprime/Drv/Interfaces/Tick.fpp) interface using **TIM2 channel 2** in output-compare "frozen" mode (`TIM_OCMODE_TIMING`) — a second, independent channel layered on the *same* free-running TIM2 counter that `Os::RawTime` already uses (see `tim2_clock.cpp`). It never touches TIM2's base counter, `ARR`, or the update/overflow interrupt the microsecond clock depends on, so the two coexist without interference.

This replaces software-polling `HAL_GetTick()` to detect elapsed milliseconds (the previous approach in `Main.cpp`) with a real hardware-interrupt-driven tick, matching the same ISR-latches-a-flag/poll-drains-it pattern already used by [`Stm32::Stm32UartDriver`](../../STM32UartDriver/docs/sdd.md).

## 2. Requirements

| Name | Description | Validation |
|---|---|---|
| STM32-TIMER-COMP-001 | Shall implement the `Drv.Tick` interface | inspection |
| STM32-TIMER-COMP-002 | Shall raise ticks from a TIM2 hardware interrupt, not software polling of another clock | inspection |
| STM32-TIMER-COMP-003 | Shall not disturb TIM2's base counter or the microsecond clock built on it | inspection |
| STM32-TIMER-COMP-004 | Shall detect and recover a compare target that elapses before it can be reprogrammed, without stalling until TIM2's ~71-minute wraparound | test |
| STM32-TIMER-COMP-005 | Shall never perform F´ port calls or allocation from ISR context | inspection |

## 3. Design

### 3.1 Port model

`import Drv.Tick` provides the single `CycleOut: Svc.Cycle` output port, wired in `topology.fpp` to `rateGroupDriver.CycleIn`. There are no input ports to implement — nothing needs to call into this component from the port graph; `open()` and `poll()` are plain public methods called directly, the same pattern used by `Stm32UartDriver::open()`/`poll()`.

### 3.2 `open(periodUs)`

Called once from `configureTopology()`, after `Stm32_Tim2ClockInit()` has already started TIM2's base counter (see `Main.cpp`). Arms TIM2 channel 2 via `HAL_TIM_OC_ConfigChannel()` (mode `TIM_OCMODE_TIMING`, so the channel has no effect on any GPIO pin) and `HAL_TIM_OC_Start_IT()`, with the first compare target set to `TIM2->CNT + periodUs` (TIM2 runs at 1 MHz, so 1 tick == 1 microsecond — no unit conversion needed). Emits `Configured` once on success.

### 3.3 `poll()`

Called every cyclic-executive pass from `Main.cpp` (not through a rate group — a 1 Hz/0.5 Hz/0.25 Hz cadence would be far too slow to service a 10 ms tick). `HAL_TIM_OC_DelayElapsedCallback()` (the ISR, dispatched by the existing shared `TIM2_IRQHandler`) only latches a `volatile` pending flag; `poll()` is where the real work happens:

1. If the flag is set: clear it, increment `TickCount`, and fire `CycleOut_out()` with a fresh `Os::RawTime` timestamp — this is the deterministic, non-ISR context where F´ port calls belong.
2. Reschedule the next compare target *relative to the target that just fired* (`m_nextTarget + periodTicks`), not relative to "now" — this keeps ticks periodic against the original schedule instead of drifting by however late `poll()` happened to run.
3. Before committing that new target, check whether `TIM2->CNT` has already passed it (a signed-difference comparison, correct across TIM2's 32-bit wraparound as long as the gap being measured is far smaller than half that span, which any real overrun is). If so, the gap between `poll()` calls was longer than one tick period — the compare would otherwise sit un-armed until the counter wraps around and reaches that value again, roughly 71 minutes later. Instead: increment `OverrunCount`, emit a throttled `TickOverrun` event, and resync the target to `now + periodTicks`.

## 4. Usage

```cpp
// configureTopology():
timer.open(10000);  // 10 ms period, matching rateGroupDivisorsSet's {100, 200, 400}

// Main.cpp cyclic executive loop, every pass:
ReferenceDeployment::timer.poll();
```

## 5. Events and telemetry

Events: `Configured`, `TickOverrun`. Telemetry: `TickCount`, `OverrunCount`.
