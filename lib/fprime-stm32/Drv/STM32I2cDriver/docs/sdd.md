# Stm32::Stm32I2cDriver

## 1. Introduction

`Stm32I2cDriver` is a bare-metal implementation of the [`Drv.I2c`](../../../fprime/Drv/Interfaces/I2c.fpp) interface for the STM32H7's I2C1 peripheral. It is a raw bus-master driver only: `write`/`read`/`writeRead` map directly onto blocking (polled) `HAL_I2C_Master_Transmit()`/`HAL_I2C_Master_Receive()` calls against whatever 7-bit address the caller supplies. There is no sensor-specific register model layered on top -- a sensor component (or a topology-level test command) is expected to drive this component's ports directly with the sensor's own register addressing convention.

## 2. Requirements

| Name | Description | Validation |
|---|---|---|
| STM32-I2C-COMP-001 | Shall implement the `Drv.I2c` interface | inspection |
| STM32-I2C-COMP-002 | Shall configure I2C1 (via `MX_I2C1_Init()`) and reject transactions issued before `open()` | inspection |
| STM32-I2C-COMP-003 | Shall perform a blocking write to a 7-bit slave address, bounded by a 10 ms transaction watchdog | inspection |
| STM32-I2C-COMP-004 | Shall perform a blocking read from a 7-bit slave address, bounded by a 10 ms transaction watchdog | inspection |
| STM32-I2C-COMP-005 | Shall perform a write followed by a read for the same address without requiring a second port invocation | inspection |
| STM32-I2C-COMP-006 | Shall distinguish an address-phase NACK (`I2C_ADDRESS_ERR`) from a data-phase/bus failure (`I2C_WRITE_ERR`/`I2C_READ_ERR`) | inspection |

## 3. Design

### 3.1 Port model

`import Drv.I2c` provides `write`/`read`/`writeRead` as guarded, synchronous input ports; each returns a `Drv::I2cStatus` to the caller directly, so there is no completion port and no telemetry. The only event is `HalError`, emitted from the HAL boundary whenever a blocking `HAL_I2C_*` call does not return `HAL_OK`.

### 3.2 Common/Real/Stub split

Following the convention in `lib/fprime-stm32/README.md`, this driver splits into three files sharing one HAL-free header (`Stm32I2cDriver.hpp`):

- `Stm32I2cDriverCommon.cpp` -- port handler logic and `open()`, all hardware-independent. Always built.
- `Stm32I2cDriver.cpp` -- the real HAL boundary (`hwOpen`/`hwMasterTransmit`/`hwMasterReceive`/`hwIsAddressNack`), built only for the `stm32h7` target. The only file allowed to include `i2c.h`/call `HAL_I2C_*`.
- `Stm32I2cDriverStub.cpp` -- the same boundary methods against injectable/observable stub state, built for host UT and any non-`stm32h7` build. Never included in a flight build.

### 3.3 `open()`

Calls `MX_I2C1_Init()` (the CubeMX-generated I2C1/GPIO/clock configuration in `lib/fprime-stm32/src/i2c.c`) and marks the driver ready. `MX_I2C1_Init()` traps in `Error_Handler()` on failure rather than returning a status, matching every other `MX_*_Init()` in this project, so `open()` cannot itself observe an init failure -- only that boot never returns.

### 3.4 `write`/`read`/`writeRead`

Each handler asserts the caller's `Fw::Buffer`(s) are non-null and fit in a `U16` (the HAL's `Size`/`DevAddress` parameter width), then calls the corresponding `hw*` boundary method. On failure, `hwIsAddressNack()` (checks `HAL_I2C_GetError() & HAL_I2C_ERROR_AF`) distinguishes "no device answered" (`I2C_ADDRESS_ERR`) from a data-phase/bus failure (`I2C_WRITE_ERR`/`I2C_READ_ERR`).

`writeRead` issues a blocking transmit immediately followed by a blocking receive to the same address -- a STOP and a fresh START, not a single electrically-held repeated START. This project's I2C1 MSP init does not enable the NVIC event/error interrupt, so the sequential IT transfer API (`HAL_I2C_Master_Seq_*_IT` with `I2C_FIRST_FRAME`/`I2C_LAST_FRAME`) needed for a true repeated START is not wired up. Because this bus has exactly one master, a STOP-then-START is safe for the common case (nothing else can interrupt the sequence); a sensor that strictly requires the bus held across the register-address write (loses its internal pointer on STOP) will not work correctly until this is upgraded.

## 4. Usage

```cpp
// configureTopology():
i2cDriver.open();

// raw sensor probe/register read, e.g. from a test command or adapter component:
Drv::I2cStatus status = i2cDriver.get_write_InputPort(0)->invoke(sensorAddr, regAddrBuffer);
status = i2cDriver.get_writeRead_InputPort(0)->invoke(sensorAddr, regAddrBuffer, readDataBuffer);
```

Note that the caller owns the buffers passed to each port; the driver does not retain or deallocate them.
