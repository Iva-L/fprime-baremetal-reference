// ======================================================================
// \title  Stm32GpioDriver.hpp
// \author ivanlara
// \brief  hpp file for the STM32H7 bare-metal GPIO pin driver
// ======================================================================

#ifndef STM32_GPIO_DRIVER_HPP
#define STM32_GPIO_DRIVER_HPP

#include <lib/fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriverComponentAc.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/DirectionEnumAc.hpp>
#include <Fw/Types/LogicEnumAc.hpp>

#include "stm32h7xx_hal.h"

namespace Drv {

class Stm32GpioDriver final : public Stm32GpioDriverComponentBase {
  public:
    
    //! Construct object Stm32GpioDriver
    explicit Stm32GpioDriver(const char* const compName);

    //! Destroy object Stm32GpioDriver
    ~Stm32GpioDriver();

    //! Enable the pin's GPIO port clock and configure the line as a push-pull
    //! output or a floating input. For an output pin, defaultState is applied
    //! before the mode switches to output, so the line never glitches through
    //! whatever level happened to be in the output register at reset.
    //! \param port: GPIO peripheral base (e.g. GPIOF)
    //! \param pin: pin bit mask (e.g. GPIO_PIN_10)
    //! \param mode: OUTPUT or INPUT
    //! \param defaultState: initial level applied before enabling an OUTPUT pin
    void open(GPIO_TypeDef* port, U16 pin, Fw::Direction mode, Fw::Logic defaultState = Fw::Logic::LOW);

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for gpioRead. Returns NOT_OPENED before open()
    //! and INVALID_MODE if the pin was configured as an output.
    Drv::GpioStatus gpioRead_handler(FwIndexType portNum, Fw::Logic& state) override;

    //! Handler implementation for gpioWrite. Returns NOT_OPENED before open()
    //! and INVALID_MODE if the pin was configured as an input.
    Drv::GpioStatus gpioWrite_handler(FwIndexType portNum, const Fw::Logic& state) override;

    GPIO_TypeDef* m_port;
    U16 m_pin;
    Fw::Direction m_mode;
    bool m_opened;
};

}  // namespace Drv

#endif