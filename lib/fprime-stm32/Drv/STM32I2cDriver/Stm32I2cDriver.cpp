// ======================================================================
// \title  Stm32I2cDriver.cpp
// \author ivanlara
// \brief  HAL boundary for the STM32H7 I2C1 blocking master driver (real
//         hardware implementation, stm32h7 target only). This is the only
//         file in this driver allowed to include i2c.h/HAL_I2C_* -- see
//         Stm32I2cDriverStub.cpp for the host unit-test stand-in.
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32I2cDriver/Stm32I2cDriver.hpp>

#include "i2c.h"

namespace Stm32 {

bool Stm32I2cDriver ::hwOpen() {
    MX_I2C1_Init();
    return true;
}

bool Stm32I2cDriver ::hwMasterTransmit(U16 devAddress, U8* data, U16 len) {
    const HAL_StatusTypeDef status =
        HAL_I2C_Master_Transmit(&hi2c1, static_cast<uint16_t>(devAddress << 1U), data, len, TRANSACTION_TIMEOUT_MS);
    if (status != HAL_OK) {
        Fw::LogStringArg _op("Master_Transmit");
        this->log_WARNING_HI_HalError(_op, devAddress, static_cast<I32>(status));
        return false;
    }
    return true;
}

bool Stm32I2cDriver ::hwMasterReceive(U16 devAddress, U8* data, U16 len) {
    const HAL_StatusTypeDef status =
        HAL_I2C_Master_Receive(&hi2c1, static_cast<uint16_t>(devAddress << 1U), data, len, TRANSACTION_TIMEOUT_MS);
    if (status != HAL_OK) {
        Fw::LogStringArg _op("Master_Receive");
        this->log_WARNING_HI_HalError(_op, devAddress, static_cast<I32>(status));
        return false;
    }
    return true;
}

bool Stm32I2cDriver ::hwIsAddressNack() {
    return (HAL_I2C_GetError(&hi2c1) & HAL_I2C_ERROR_AF) != 0U;
}

}  // namespace Stm32
