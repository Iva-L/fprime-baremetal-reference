#ifndef STM32_UART_DRIVER_HPP
#define STM32_UART_DRIVER_HPP
#include <fprime-stm32/Components/Stm32UartDriver/UartDriverComponentAc.hpp>
namespace Stm32 {
class UartDriver final : public UartDriverComponentBase {
public:
  explicit UartDriver(const char* name = "") : UartDriverComponentBase(name) {}
protected:
  void recvReturnIn_handler(FwIndexType, Fw::Buffer&) override {}
  void run_handler(FwIndexType, U32) override {}
  Drv::ByteStreamStatus send_handler(FwIndexType, Fw::Buffer&) override { return Drv::ByteStreamStatus::OTHER_ERROR; }
};
}
#endif
