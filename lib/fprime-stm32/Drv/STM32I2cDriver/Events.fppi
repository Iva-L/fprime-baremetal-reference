@ A blocking I2C1 HAL call returned a non-OK status
event HalError(
                   op: string size 24 @< The HAL operation that failed
                   addr: U32 @< The 7-bit I2C slave address involved
                   status: I32 @< The HAL_StatusTypeDef value
                 ) \
  severity warning high \
  id 0 \
  format "I2C1 HAL operation {} to address 0x{x} failed: {}" \
  throttle 1 every { seconds = 10 }
