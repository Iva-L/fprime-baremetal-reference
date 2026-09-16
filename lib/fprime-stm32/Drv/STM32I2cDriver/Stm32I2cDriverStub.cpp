// ======================================================================
// \title  Stm32I2cDriverStub.cpp
// \brief  HAL boundary stand-in for host-native unit tests. No I2C1
//         hardware exists on the build host, so every call here operates
//         on injectable/observable stub state instead of real registers --
//         no HAL/CMSIS include, no register access. A unit test drives
//         success/failure via the extern stub state below instead of
//         relying on real HAL_I2C_* return codes.
// ======================================================================

#include <lib/fprime-stm32/Drv/STM32I2cDriver/Stm32I2cDriver.hpp>

// Injectable stub state for unit tests
extern bool Stub_hwOpenSucceeds = true;               // simulates MX_I2C1_Init()
extern bool Stub_hwMasterTransmitSucceeds = true;     // simulates HAL_I2C_Master_Transmit() == HAL_OK
extern bool Stub_hwMasterReceiveSucceeds = true;      // simulates HAL_I2C_Master_Receive() == HAL_OK
extern bool Stub_hwAddressNack = false;               // simulates HAL_I2C_GetError() & HAL_I2C_ERROR_AF
extern U8 Stub_readResponseData[32] = {0};        // bytes hwMasterReceive() copies into the caller's buffer

// Observable stub state for unit tests
extern U16 Stub_lastDevAddress = 0;
extern U8 Stub_lastWriteData[32] = {0};
extern U16 Stub_lastWriteLen = 0;
extern U16 Stub_lastReadLen = 0;

namespace Stm32 {

bool Stm32I2cDriver ::hwOpen() {
    return Stub_hwOpenSucceeds;
}

bool Stm32I2cDriver ::hwMasterTransmit(U16 devAddress, U8* data, U16 len) {
    Stub_lastDevAddress = devAddress;

    const FwSizeType captured =
        (static_cast<FwSizeType>(len) < sizeof(Stub_lastWriteData)) ? static_cast<FwSizeType>(len) : sizeof(Stub_lastWriteData);
    for (FwSizeType i = 0; i < captured; i++) {
        Stub_lastWriteData[i] = data[i];
    }
    Stub_lastWriteLen = static_cast<U16>(captured);

    return Stub_hwMasterTransmitSucceeds;
}

bool Stm32I2cDriver ::hwMasterReceive(U16 devAddress, U8* data, U16 len) {
    Stub_lastDevAddress = devAddress;
    Stub_lastReadLen = len;

    const FwSizeType toCopy =
        (static_cast<FwSizeType>(len) < sizeof(Stub_readResponseData)) ? static_cast<FwSizeType>(len) : sizeof(Stub_readResponseData);
    for (FwSizeType i = 0; i < toCopy; i++) {
        data[i] = Stub_readResponseData[i];
    }

    return Stub_hwMasterReceiveSucceeds;
}

bool Stm32I2cDriver ::hwIsAddressNack() {
    return Stub_hwAddressNack;
}

}  // namespace Stm32
