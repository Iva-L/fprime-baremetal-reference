// ======================================================================
// \title  Stm32GpioDriver.cpp
// \author ivanlara
// \brief  cpp file for the STM32H7 bare-metal GPIO pin driver
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriver.hpp>

namespace {

//! Enable the AHB4 clock for whichever GPIO port is passed in
void enableGpioClock(GPIO_TypeDef* port) {

    FW_ASSERT(port != nullptr);

    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    } else if (port == GPIOF) {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    } else if (port == GPIOG) {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    } else if (port == GPIOH) {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    } else if (port == GPIOI) {
        __HAL_RCC_GPIOI_CLK_ENABLE();
    } else if (port == GPIOJ) {
        __HAL_RCC_GPIOJ_CLK_ENABLE();
    } else if (port == GPIOK) {
        __HAL_RCC_GPIOK_CLK_ENABLE();
    } else {
        FW_ASSERT(false, reinterpret_cast<FwAssertArgType>(port));
    }
}

}  // namespace

namespace Stm32 {

Stm32GpioDriver ::Stm32GpioDriver(const char* const compName)
    : Stm32GpioDriverComponentBase(compName),
      m_port(nullptr),
      m_pin(0),
      m_mode(Fw::Direction::IN),
      m_opened(false) {}

Stm32GpioDriver ::~Stm32GpioDriver() {}

Fw::Success Stm32GpioDriver ::open(GPIO_TypeDef* port, U16 pin, Fw::Direction mode, Fw::Logic defaultState) {
    FW_ASSERT(port != nullptr);

    enableGpioClock(port);

    if (mode == Fw::Direction::OUT) {
        HAL_GPIO_WritePin(port, pin, (defaultState == Fw::Logic::HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    GPIO_InitTypeDef init = {};
    init.Pin = pin;
    init.Mode = (mode == Fw::Direction::OUT) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &init);

    // Verify that the GPIO was successfully configured for the specified mode.
    const U32 position = POSITION_VAL(pin);
    const U32 actualModeBits = (port->MODER >> (position * 2U)) & GPIO_MODE;
    const U32 expectedModeBits = init.Mode & GPIO_MODE;
    if (actualModeBits != expectedModeBits) {
        this->log_WARNING_HI_ConfigureError(pin, mode);
        this->m_opened = false;
        return Fw::Success::FAILURE;
    }

    this->m_port = port;
    this->m_pin = pin;
    this->m_mode = mode;
    this->m_opened = true;
    return Fw::Success::SUCCESS;
}

Drv::GpioStatus Stm32GpioDriver ::gpioRead_handler(FwIndexType portNum, Fw::Logic& state) {
    if (!this->m_opened) {
        return Drv::GpioStatus::NOT_OPENED;
    }
    if (this->m_mode != Fw::Direction::IN) {
        return Drv::GpioStatus::INVALID_MODE;
    }
    state = (HAL_GPIO_ReadPin(this->m_port, this->m_pin) == GPIO_PIN_SET) ? Fw::Logic::HIGH : Fw::Logic::LOW;
    return Drv::GpioStatus::OP_OK;
}

Drv::GpioStatus Stm32GpioDriver ::gpioWrite_handler(FwIndexType portNum, const Fw::Logic& state) {
    if (!this->m_opened) {
        return Drv::GpioStatus::NOT_OPENED;
    }
    if (this->m_mode != Fw::Direction::OUT) {
        return Drv::GpioStatus::INVALID_MODE;
    }
    HAL_GPIO_WritePin(this->m_port, this->m_pin, (state == Fw::Logic::HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return Drv::GpioStatus::OP_OK;
}

}  // namespace Stm32
