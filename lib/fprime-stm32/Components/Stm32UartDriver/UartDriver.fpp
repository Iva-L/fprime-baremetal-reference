module Stm32 {
  passive component UartDriver {
    import Drv.ByteStreamDriver
    output port allocate: Fw.BufferGet
    output port deallocate: Fw.BufferSend
    sync input port run: Svc.Sched
    event port Log
    telemetry port Tlm
    text event port LogText
    time get port Time
  }
}
