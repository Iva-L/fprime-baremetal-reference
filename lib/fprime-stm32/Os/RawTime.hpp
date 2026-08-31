// ======================================================================
// \title Os/RawTime.hpp
// \brief STM32H7 HAL-tick implementation of Os::RawTime
// ======================================================================
#ifndef FPRIME_STM32_OS_RAWTIME_HPP
#define FPRIME_STM32_OS_RAWTIME_HPP

#include <Os/RawTime.hpp>

namespace Os {
namespace Stm32 {
namespace RawTime {

struct Stm32RawTimeHandle : public RawTimeHandle {
    U32 m_milliseconds = 0;
};

class Stm32RawTime final : public RawTimeInterface {
  public:
    Stm32RawTime() = default;
    ~Stm32RawTime() override = default;

    RawTimeHandle* getHandle() override;
    Status now() override;
    Status getTimeInterval(const Os::RawTime& other, Fw::TimeInterval& interval) const override;
    Fw::SerializeStatus serializeTo(Fw::SerialBufferBase& buffer,
                                    Fw::Endianness mode = Fw::Endianness::BIG) const override;
    Fw::SerializeStatus deserializeFrom(Fw::SerialBufferBase& buffer,
                                        Fw::Endianness mode = Fw::Endianness::BIG) override;

  private:
    Stm32RawTimeHandle m_handle;
};

}  // namespace RawTime
}  // namespace Stm32
}  // namespace Os

#endif  // FPRIME_STM32_OS_RAWTIME_HPP