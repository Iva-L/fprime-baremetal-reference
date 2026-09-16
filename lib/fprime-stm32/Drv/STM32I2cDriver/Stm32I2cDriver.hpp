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

class Stm32I2cDriver final : public Stm32I2cDriverComponentBase {
  public:
    //! Construct Stm32I2cDriver object
    Stm32I2cDriver(const char* const compName  //!< The component name
    );

    //! Destroy Stm32I2cDriver object
    ~Stm32I2cDriver();

    //! Run MX_I2C1_Init() and mark the driver ready to accept transactions.
    //! Must be called once from configureTopology(), before any
    //! write/read/writeRead port invocation.
    Fw::Success open();

  private:
    //! Bounded per-transaction watchdog passed to every blocking HAL_I2C_*
    //! call (Checklist Week 10: "10 ms transaction watchdog").
    static constexpr U32 TRANSACTION_TIMEOUT_MS = 10;

    //! Run MX_I2C1_Init(). CubeMX-generated init traps in Error_Handler()
    //! on failure rather than returning a status (matches every other
    //! MX_*_Init() in this project), so there is nothing to check here.
    bool hwOpen();

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
