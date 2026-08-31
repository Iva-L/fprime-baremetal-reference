// ======================================================================
// \title Os/RawTime.cpp
// \brief STM32H7 HAL-tick implementation of Os::RawTime
// ======================================================================
#include "RawTime.hpp"
#include <stm32h7xx_hal.h>

namespace Os {
namespace Stm32 {
namespace RawTime {

RawTimeHandle* Stm32RawTime::getHandle() {
    return &this->m_handle;
}

RawTimeInterface::Status Stm32RawTime::now() {
    this->m_handle.m_milliseconds = HAL_GetTick();
    return Status::OP_OK;
}

RawTimeInterface::Status Stm32RawTime::getTimeInterval(const Os::RawTime& other,
                                                        Fw::TimeInterval& interval) const {
    const U32 otherMilliseconds =
        static_cast<Stm32RawTimeHandle*>(const_cast<Os::RawTime&>(other).getHandle())->m_milliseconds;
    const U32 elapsedMilliseconds = this->m_handle.m_milliseconds - otherMilliseconds;
    interval.set(elapsedMilliseconds / 1000U, (elapsedMilliseconds % 1000U) * 1000U);
    return Status::OP_OK;
}

Fw::SerializeStatus Stm32RawTime::serializeTo(Fw::SerialBufferBase& buffer, Fw::Endianness mode) const {
    static_assert(Stm32RawTime::SERIALIZED_SIZE >= 2 * sizeof(U32),
                  "Stm32RawTime requires at least two U32 serialization fields");
    Fw::SerializeStatus status = buffer.serializeFrom(this->m_handle.m_milliseconds / 1000U, mode);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return status;
    }
    return buffer.serializeFrom((this->m_handle.m_milliseconds % 1000U) * 1000U, mode);
}

Fw::SerializeStatus Stm32RawTime::deserializeFrom(Fw::SerialBufferBase& buffer, Fw::Endianness mode) {
    U32 seconds = 0;
    U32 microseconds = 0;
    Fw::SerializeStatus status = buffer.deserializeTo(seconds, mode);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return status;
    }
    status = buffer.deserializeTo(microseconds, mode);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return status;
    }
    if (microseconds >= 1000000U || seconds > (0xFFFFFFFFU / 1000U)) {
        return Fw::SerializeStatus::FW_DESERIALIZE_FORMAT_ERROR;
    }
    this->m_handle.m_milliseconds = seconds * 1000U + microseconds / 1000U;
    return Fw::SerializeStatus::FW_SERIALIZE_OK;
}

}  // namespace RawTime
}  // namespace Stm32
}  // namespace Os