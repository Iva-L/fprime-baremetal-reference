// ======================================================================
// \title  Stm32I2cDriver.hpp
// \author ivanlara
// \brief  hpp file for the STM32H7 I2C1 blocking master driver
// ======================================================================

#ifndef Stm32_Stm32I2cDriver_HPP
#define Stm32_Stm32I2cDriver_HPP

#include "lib/fprime-stm32/Drv/STM32I2cDriver/Stm32I2cDriverComponentAc.hpp"
#include <Fw/Types/SuccessEnumAc.hpp>

namespace Stm32 {

//! I2C peripheral identifier, covering every instance present on an STM32H753.
//! Only I2c1 has a CubeMX-generated MX_I2Cn_Init()/handle in this project
//! today (see src/i2c.c/include/i2c.h) -- the others are declared so a
//! different board/peripheral selection only requires adding that CubeMX
//! config and one switch case in Stm32I2cDriver.cpp, not touching this
//! header, Common.cpp, or the Stub.
enum class I2cInstance { I2c1, I2c2, I2c3, I2c4 };

//! Supported I2C bus clock presets. Unlike UART's baud rate, the STM32H7's
//! I2C Timing register has no closed-form runtime formula in the HAL -- each
//! value is a CubeMX-computed constant for this project's actual peripheral
//! clock (D2PCLK1), not derived at runtime, so only the presets CubeMX has
//! actually generated for this project are offered here.
enum class I2cBusSpeed { Standard, Fast, FastPlus };

class Stm32I2cDriver final : public Stm32I2cDriverComponentBase {
  public:
    //! Construct Stm32I2cDriver object
    Stm32I2cDriver(const char* const compName  //!< The component name
    );

    //! Destroy Stm32I2cDriver object
    ~Stm32I2cDriver();

    //! Configure the selected I2C instance (via the CubeMX-generated
    //! MX_I2Cn_Init(), then apply the requested bus speed preset) and mark
    //! the driver ready to accept transactions. Must be called once from
    //! configureTopology(), before any write/read/writeRead port
    //! invocation.
    //! \param instance which I2C peripheral this driver instance owns
    //! \param busSpeed requested bus clock preset (default matches this
    //!        project's current 400 kHz Fast-mode configuration)
    Fw::Success open(I2cInstance instance, I2cBusSpeed busSpeed = I2cBusSpeed::Fast);

  private:
    //! Bounded per-transaction watchdog passed to every blocking HAL_I2C_*
    //! call (Checklist Week 10: "10 ms transaction watchdog").
    static constexpr U32 TRANSACTION_TIMEOUT_MS = 10;

    //! Run MX_I2Cn_Init() for the selected instance and apply the requested
    //! bus speed preset (re-running HAL_I2C_Init() if it differs from
    //! CubeMX's baked-in default). No NVIC configuration: this driver is
    //! polled-only. CubeMX-generated init traps in Error_Handler() on
    //! failure rather than returning a status (matches every other
    //! MX_*_Init() in this project), so only the speed-override HAL_I2C_Init()
    //! call has a real failure path here.
    bool hwOpen(I2cInstance instance, I2cBusSpeed busSpeed);

    //! Blocking (polled) master write of `len` bytes to `devAddress`,
    //! bounded by TRANSACTION_TIMEOUT_MS. Returns true if the HAL reported
    //! HAL_OK; emits HalError itself on failure, since only this method
    //! knows the raw HAL_StatusTypeDef.
    bool hwMasterTransmit(U16 devAddress, U8* data, U16 len);

    //! Blocking (polled) master read of `len` bytes from `devAddress`,
    //! bounded by TRANSACTION_TIMEOUT_MS. Same return/error-reporting
    //! convention as hwMasterTransmit().
    bool hwMasterReceive(U16 devAddress, U8* data, U16 len);

    //! True if the most recently failed hwMasterTransmit()/hwMasterReceive()
    //! call failed on an address-phase NACK (no device answered), as
    //! opposed to a data-phase or bus error.
    bool hwIsAddressNack();

    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Guarded synchronous write. Returns I2C_OPEN_ERR before open().
    Drv::I2cStatus write_handler(const FwIndexType portNum, U32 addr, Fw::Buffer& serBuffer) override;

    //! Guarded synchronous read. Returns I2C_OPEN_ERR before open().
    Drv::I2cStatus read_handler(const FwIndexType portNum, U32 addr, Fw::Buffer& serBuffer) override;

    //! Guarded write-then-read. NOTE: implemented as two back-to-back
    //! blocking HAL_I2C_Master_Transmit()/HAL_I2C_Master_Receive() calls
    //! (STOP then START), not a single electrically-held repeated START --
    //! see Stm32I2cDriverCommon.cpp for why that is acceptable on this
    //! single-master bus and what would need to change to hold the bus.
    Drv::I2cStatus writeRead_handler(const FwIndexType portNum,
                                      U32 addr,
                                      Fw::Buffer& writeBuffer,
                                      Fw::Buffer& readBuffer) override;

    bool m_opened;
};

}  // namespace Stm32

#endif
