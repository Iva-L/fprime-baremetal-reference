# Fprime Baremetal Reference (STM32H7 F´)

This project is an implementation of F´ on baremetal hardware, specifically for STM32H7 based microcontrollers.

The BaremetalReference is a reference to run F´ on embedded systems with hardware constraints. Therefore, this deployment only consists of basic/essential F´ components.

F´ (F Prime) is a component-driven framework that enables rapid development and deployment of spaceflight and other embedded software applications.
**Please Visit the F´ Website:** https://fprime.jpl.nasa.gov.

## STM32H7 build

The project uses the GNU Arm Embedded toolchain and the `stm32h7` F´ platform
for the STM32H753XI-EVAL2 target. The platform selects the cooperative
`fprime-baremetal` OS implementations and disables POSIX and socket support.

```shell
source fprime-venv/bin/activate
fprime-util generate stm32h7 --build-cache build-fprime-stm32h7
fprime-util build --build-cache build-fprime-stm32h7 -j"$(nproc)"
```

The current build validates the framework and bare-metal libraries. A
flashable image still requires the board startup code, linker script, cyclic
executive `main()`, and STM32 USART1 DMA driver.
