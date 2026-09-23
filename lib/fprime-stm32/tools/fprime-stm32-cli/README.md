# fprime-stm32-cli

Ports an STM32CubeMX-generated STM32H7 project's linker script and startup
assembly file into this repo's F' bare-metal `Hardware/` layout.

## Install

```bash
pip install -e lib/fprime-stm32/tools/fprime-stm32-cli
```

## Usage

Run from the root of an F' deployment project (a directory containing
`Hardware/`, or containing exactly one `<Deployment>/Hardware/`):

```bash
fprime-stm32 sync path/to/cubemx_project [--dry-run] [--deployment <name>]
```

This locates the CubeMX-generated `*_FLASH.ld` and `startup_stm32h7*.s`
files, patches them, and writes the results to `Hardware/linker/` and
`Hardware/startup/`.

## What "porting" means here

The patch mirrors the hand-edit already validated on hardware in this repo
(compare `FprimeBaremetalReference/Hardware/stm32h753_hal/` — the raw CubeMX
output — against `FprimeBaremetalReference/Hardware/{linker,startup}/`):

- Renames the CubeMX RAM regions: `DTCMRAM` -> `DTCM_RAM`, the D1-domain AXI
  SRAM bank (origin `0x24000000`, whatever CubeMX calls it) -> `AXI_SRAM`.
- Retargets the default `.data`/`.tdata`/`.tbss`/`.bss` output sections from
  DTCM to `AXI_SRAM`, so `Fw::Buffer` storage and the `BootstrapAllocator`
  static pool (both plain `.bss`, see
  `fprime-stm32/Allocator/BootstrapAllocator.cpp`) land in DMA-safe memory
  without any source change.
- Carves out a `.dtcm_bss` NOLOAD section (`DTCM_RAM`, 32-byte aligned) with
  `_sdtcm_bss`/`_edtcm_bss` boundary symbols, for CPU-only globals tagged
  `__attribute__((section(".dtcm_bss")))` (see
  `Hardware/config/include/PlatformMemory.hpp`'s `ATTR_DTCM_BSS`).
- Appends an explicit `.axi_sram` NOLOAD section (`AXI_SRAM`, 32-byte
  aligned) with `_saxi_sram`/`_eaxi_sram` boundary symbols, for code that
  wants to opt a static allocation into AXI SRAM explicitly via a matching
  `ATTR_AXI_SRAM` attribute. This section is additional to, not a
  replacement for, the default `.data`/`.bss` remap above.
- In the startup file, inserts a zero-fill loop for `.dtcm_bss` right after
  the standard CubeMX `.bss` zero-fill loop, then a call to
  `Stm32_registerBootstrapAllocator` before any C++ static constructor can
  run. This assumes the deployment links `fprime-stm32`'s
  `Allocator/BootstrapAllocator.cpp` — if it doesn't, the link step will
  fail on the undefined symbol.

The tool refuses to patch (raises a clear error instead of guessing) if it
doesn't recognize the standard CubeMX linker/startup template, or if the
linker script has no DTCM/AXI SRAM region at their fixed STM32H7 silicon
addresses (`0x20000000` / `0x24000000`).

## Tests

```bash
pip install -e '.[test]'
pytest
```

Fixtures under `tests/fixtures/cubemx_stm32h753/` are literal copies of the
real raw CubeMX output already in this repo
(`FprimeBaremetalReference/Hardware/stm32h753_hal/`).
