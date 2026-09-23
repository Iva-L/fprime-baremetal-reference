# F´ Project Tutorial for STM32-Supported MCUs

>[!IMPORTANT]
> This project showcases the STM32H753XIH6 running on a STM32H753I-EVAL2 board as a worked example. Peripheral choice, pins, and clock configuration are always board-specific and always require your own CubeMX configuration - but the `fprime-stm32 sync` step that wires CubeMX's generated code into the F' CMake build works for any STM32H7 chip/board, not just this one.

This tutorial is meant for F´ users who want to create and develop an F´ project for any of the F´ supported STM32 microcontrollers. If you are a new F´ user seeking to learn about the basics of creating components, using events, telemetry, commands, and parameters, and integrating topologies with the goal of running F´ on embedded hardware, we recommend taking the [LED Blinker tutorial](https://fprime.jpl.nasa.gov/latest/tutorials-led-blinker/docs/led-blinker) first.

This tutorial will allow you to create, build and deploy your own F´ project on any of the supported STM32 family 32-bit microcontrollers based on the Arm Cortex®-M processor using the `fprime-stm32` and [`fprime-baremetal`](https://github.com/fprime-community/fprime-baremetal) libraries.

> [!TIP]
> The Reference Project for this tutorial is located the [Fprime Baremetal Reference Project](https://github.com/Iva-L/fprime-baremetal-reference/tree/library-fprime-stm32). If you are stuck at some point during this tutorial, you may refer to that reference as the "solution".

## Prerequisites

In order to run through this tutorial, users should first have the following prerequisites ready:

1. Meet the [F´ System Requirements](https://github.com/nasa/fprime?tab=readme-ov-file#system-requirements).
2. Have [F' installed](https://fprime.jpl.nasa.gov/latest/docs/getting-started/installing-fprime/) already.
3. An IDE or text editor supporting copy-paste.
> [!TIP]
> Using an IDE or text editor with FPP language support can significantly improve your development experience. [VSCode](https://code.visualstudio.com/) has [extensions](https://marketplace.visualstudio.com/items?itemName=jet-propulsion-laboratory.fpp) that support the FPP language syntax.
5. Acquire any familiy-supported STM32 board.
6. Have [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) initialization code generator installed for STM32 hardware configuration.
> [!IMPORTANT]
> If you do not have the hardware yet, you can still follow this tutorial! You should just skip the Hardware sections.

## Troubleshooting
If at any point during this tutorial you encounter issues:

1. **Check your current directory:** Ensure you are in the correct directory as specified in each step of the tutorial.
2. **Activate your virtual environment:** Always make sure your F´ project's virtual environment is activated with `. fprime-venv/bin/activate`.
3. **Refer to the F´ troubleshooting guide:** Visit [F´ Installation and Troubleshooting](https://fprime.jpl.nasa.gov/latest/docs/getting-started/installing-fprime/#troubleshooting) for common installation and setup issues.
4. **Verify your F´ installation:** Run `fprime-util --help` to ensure F´ tools are properly installed.
5. **Verify your build environment:** Ensure that your project's build directory is clean and properly configured before attempting to build your F´ project. You can purge the CMake cache by running `fprime-util purge` in your project's build directory.
6. **Verify your toolchain:** Ensure that the toolchain required for building your F´ project is correctly installed and accessible in your environment. Also, check you are passing the correct toolchain name to your build commands, as in `fprime-util build <toolchain-name>`.


## Tutorial Steps

1. [Project Setup](#1-project-setup)

---

## 1. Project Setup

> [!NOTE]
> If you have followed the [LED Blinker tutorial](https://fprime.jpl.nasa.gov/latest/tutorials-led-blinker/docs/led-blinker) previously, this should feel very familiar...

An F´ Project ties to a specific version of tools to work with F´. In order to create
this project and install the correct version of tools, you should perform a bootstrap of F´:

1. Ensure you meet the [F´ System Requirements](https://github.com/nasa/fprime?tab=readme-ov-file#system-requirements)
2. [Bootstrap your F´ project](https://fprime.jpl.nasa.gov/latest/docs/getting-started/installing-fprime/#creating-a-new-f-project) with the name of your project. In this tutorial we will use `stm32h7-project` as the project name and `Stm32h7Project` for the project namespace.

Bootstrapping your F´ project created a folder called `stm32h7-project` (or any name you chose) containing the standard F´ project structure as well as the virtual environment up containing the tools to work with F´.

Navigate to your project directory and activate your virtual environment if you have not already done so:
```sh
cd stm32h7-project
. fprime-venv/bin/activate
```

## 3. Installing the ARM GNU Toolchain

The `fprime-stm32` library uses the `ARM GNU Toolchain`, so before building your project for STM32 MCUs it is essential to have this toolchain installed and properly configured. Please ensure that you have the toolchain installed and accessible in your environment.

If you already have the ARM GNU Toolchain installed, you can skip this step. Otherwise, follow the instructions below to install it.

For Ubuntu-based systems, you can install the ARM GNU Toolchain using the following commands:
```sh
# Update package lists and install the ARM GNU Toolchain
sudo apt-get update

# Install the ARM GNU Toolchain
sudo apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-dev
```
> [!IMPORTANT]
> If you are using a different operating system, refer to the official [ARM GNU Toolchain installation guide](https://learn.arm.com/install-guides/gcc/arm-gnu/) for your platform.

Finally, verify the installation by checking the version of the ARM GNU Toolchain:
```sh
arm-none-eabi-gcc --version
arm-none-eabi-g++ --version
```
## 4. Using `fprime-stm32`

The `fprime-stm32` package is dependent on the `fprime-baremetal` package, which provides the core support for building F´ projects on bare-metal platforms. Ensure that the `fprime-baremetal` package is included in your project before using `fprime-stm32`.

For this reason, you should also add the `fprime-baremetal` package as a submodule inside the `\lib` directory of your project if it is not already included:
```sh
# In stm32h7-project
git submodule add https://github.com/fprime-community/fprime-baremetal.git lib/fprime-baremetal
git submodule update --init --recursive
```

Now, the `fprime-stm32` package provides the necessary support for building F´ projects targeting STM32 microcontrollers. Ensure that this package is included in your project and properly configured before attempting to build for STM32.

Now, add the `fprime-stm32` package as a submodule inside the `\lib` directory of your project if it is not already included:
```sh
git submodule add https://github.com/fprime-community/fprime-stm32.git lib/fprime-stm32 # Update this URL when fprime-stm32 library releases
git submodule update --init --recursive
```

Finally, edit the `settings.init` file in your root project directory to include the STM32 build target and relevant configurations.

To do so add the following line bellow `framework_path: ./lib/fprime`:

```ini
library_locations: ./lib/fprime-baremetal:./lib/fprime-stm32
```

## 5. Adding the /Hardware Directory

The `/Hardware` directory is used to store hardware-specific configurations and drivers for your STM32 project. To add this directory to your project, copy the hardware template directory of your board's family MCU from the `fprime-stm32` package:

```sh
# In Root Project Directory
cp -r lib/fprime-stm32/fprime-stm32/Templates/stm32h7/Hardware Stm32h7Project
```

Inside the `/Hardware` directory, you can find the hardware-specific configurations for your STM32 project. You can now modify these files according to your project's requirements.

You'll also need the `fprime-stm32` CLI tool, which wires a CubeMX-generated project into this `/Hardware` directory and the F' CMake build for you:

```sh
# In Root Project Directory
pip install -e lib/fprime-stm32/tools/fprime-stm32-cli
```

In the next step we will be creating a STM32CubeMX project for your STM32 MCU to configure the peripherals and generate the initialization code.

## 6. Creating a STM32CubeMX Project

STM32CubeMX will generate the necessary HAL (Hardware Abstraction Layer) libraries and initialization code for your STM32 MCU. F´ will need these HAL libraries and initialization code to properly interface with the hardware.

Steps to Create a STM32CubeMX Project:

1. Open STM32CubeMX and create a new project for your STM32 MCU.
2. Configure the peripherals and middleware according to your project requirements.
3. In Project Manager -> Project, set **Toolchain/IDE** to **CMake**. This matters: it's what makes CubeMX generate `cmake/stm32cubemx/CMakeLists.txt`, which `fprime-stm32 sync` reads to learn which chip, HAL modules, and peripheral-init files your project actually needs.
4. Generate the project directly into a new subdirectory of `Hardware/`, named after your chip (e.g. `Hardware/stm32h743_hal/` for an STM32H743). Generate the initialization code and HAL libraries.
5. Run `fprime-stm32 sync Hardware/<name>_hal` to wire that generated project into the F' CMake build. You never hand-copy or hand-edit `Hardware/CMakeLists.txt`, `Hardware/linker/`, or `Hardware/startup/` yourself.

> [!NOTE]
> If you have worked with STM32CubeMX before, this process should feel familiar...
> But if you are new to STM32CubeMX, follow the next creation example carefully to set up your project correctly.

Here is an example of how to create a STM32CubeMX project for the STM32H753XIH6 MCU running on a STM32H753I-EVAL2 development board:

## 7. Board Selection

### 7a. Open STM32CubeMX and select "ACCESS TO MCU SELECTOR".

![Main STM32CubeMX Window](img/mcu-selector.png)

> [!NOTE]
> If you will be working on a commercial STM32 development board, select "ACCESS TO BOARD SELECTOR" and make sure to select the correct board to ensure proper configuration and initialization.

### 7b. Choose your specific STM32H7 MCU or development board and click "Start Project". For this project, we will be using the STM32H753XIH6 MCU.

![STM32H753XIH6 MCU Selection](img/chip-selection.png)

> [!NOTE]
> This MCU has a Memory Protection Unit (MPU) and supports various high-speed peripherals, making it suitable for complex embedded applications. Select yes when prompted after clicking "Start Project" to enable the MPU.

## 8. Peripheral, Clock, and Middleware, Configuration

### 8a. One GPIO pin as an output to control an LED **(GPIO_PORT_F, GPIO_PIN_10)**. 
* For this, select pin PF10 in the pinout view and configure it as an output.
![GPIO Pin Configuration](img/pf10-gpio.png)


### 8b. One UART interface for serial communication **(USART1)**.
> It is important to configure the UART interface exactly as specified to ensure proper communication with the gds system.
* First go to the "Connectivity" section in STM32CubeMX and select "USART1" to configure the UART interface. Here select the mode as "Asynchronous" and disable hardware flow control (RS232).

* Is necessary for `USART1` to have DMA enabled for efficient data transfer. As this port will be used for communication with the gds system, make sure to enable DMA for both TX (`DMA_Stream_0`) and RX (`DMA_Stream_1`) channels in the STM32CubeMX configuration by clicking the "ADD" button.
![USART1 DMA Configuration](img/usart1-dma.png)

* `USART1_TX` will be configured on pin `PB14`, as we are working on the STM32H753I-EVAL2 development board, and this pin corresponds to the TX function for USART1 on this board.
![USART1 TX Pin Configuration](img/usart1-tx-pin.png)

* `USART1_RX` will be configured on pin `PB15`, as this pin corresponds to the RX function for USART1 on the STM32H753I-EVAL2 development board.
![USART1 RX Pin Configuration](img/usart1-rx-pin.png)

### 8c. One Timer peripheral for the custom microsecond clock **(TIM2)**.

### 8d. Generate the project directly into `Hardware/stm32h753_hal/` (Toolchain/IDE must be set to "CMake").

### 8e. Run `fprime-stm32 sync` to wire the generated project into the F' CMake build:

```sh
# In Root Project Directory
fprime-stm32 sync Hardware/stm32h753_hal
```

This patches the CubeMX-generated linker script and startup file for F´'s bare-metal zero-dynamic-memory architecture (DMA-safe buffers in AXI SRAM, CPU-only state in DTCM), writes them to `Hardware/linker/` and `Hardware/startup/`, and regenerates `Hardware/CMakeLists.txt` from CubeMX's own `cmake/stm32cubemx/CMakeLists.txt` so the `FprimeStm32` library target picks up exactly the HAL sources/includes/defines your peripheral configuration needs - for any STM32H7 chip, not just the STM32H753XIH6 used in this example. Add `--dry-run` first to preview the changes.

`sync` also exports three CMake cache variables from `Hardware/CMakeLists.txt` - `FPRIME_STM32_LINKER_SCRIPT`, `FPRIME_STM32_STARTUP_SOURCE`, `FPRIME_STM32_IT_SOURCE` - so your deployment's `CMakeLists.txt` never needs to hardcode a chip-specific filename. Reference them once when you set up your deployment:

```cmake
register_fprime_deployment(
    YourDeployment
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
        "${FPRIME_STM32_STARTUP_SOURCE}"
        "${FPRIME_STM32_IT_SOURCE}"
    ...
)

target_link_options(YourDeployment PRIVATE
    "-T${FPRIME_STM32_LINKER_SCRIPT}"
    ...
)
```

Re-run `fprime-stm32 sync` any time you regenerate `Hardware/stm32h753_hal/` from CubeMX (e.g. after adding a peripheral) - it re-derives everything from the current CubeMX output and preserves any project-specific linker placement rules you've hand-added since the last sync (e.g. pinning a specific symbol into DTCM).

## 7. Building the Project for STM32

Once the ARM GNU Toolchain is installed and verified and the necessary submodules and directories are added, you can generate and build your F´ project for STM32 microcontrollers using the following commands:
```sh
fprime-util generate stm32h7
fprime-util build stm32h7
```

This will compile your project for the STM32 target using the specified toolchain. To setup this toolchain as your default build target, you can add the following line into your `settings.init` just bellow the `library_locations` line we added last step:

```
default_toolchain: stm32h7
```

Now to build your project for the STM32 target, simply run the the same commands as before without specifying the target explicitly:
```sh
fprime-util generate 
fprime-util build 
```


