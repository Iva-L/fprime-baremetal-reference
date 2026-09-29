// ======================================================================
// \title  Stm32SpiDriver.hpp
// \author ivanlara
// \brief  hpp file for Stm32SpiDriver component implementation class
// ======================================================================

#ifndef Stm32_Stm32SpiDriver_HPP
#define Stm32_Stm32SpiDriver_HPP

#include "fprime-Stm32/Drv/Stm32SpiDriver/Stm32SpiDriverComponentAc.hpp"

namespace Stm32 {

class Stm32SpiDriver final : public Stm32SpiDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct Stm32SpiDriver object
    Stm32SpiDriver(const char* const compName  //!< The component name
    );

    //! Destroy Stm32SpiDriver object
    ~Stm32SpiDriver();
};

}  // namespace Stm32

#endif
