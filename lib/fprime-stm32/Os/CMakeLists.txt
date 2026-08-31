
####
# STM32H7 bare-metal OSAL implementations.
####

register_os_implementation(Task Stm32 FprimeStm32 fprime-baremetal_Os_TaskRunner)
register_os_implementation(Mutex Stm32 FprimeStm32)
register_os_implementation(RawTime Stm32 FprimeStm32)