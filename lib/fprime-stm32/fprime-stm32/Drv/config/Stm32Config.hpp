// ======================================================================
// \title  Stm32Config.hpp
// \author ivanlara
// \brief  Configuration file for STM32 peripherals
// ======================================================================
#ifndef STM32_CONFIG_HPP
#define STM32_CONFIG_HPP

// ======================================================================
// USART/UART peripheral instances
// ======================================================================

#ifndef USART1_UART_INSTANCE
#define USART1_UART_INSTANCE (true) //!< Indicates whether the USART1 peripheral instance is enabled
#endif  // USART1_UART_INSTANCE

#ifndef USART2_UART_INSTANCE
#define USART2_UART_INSTANCE (false) //!< Indicates whether the USART2 peripheral instance is enabled
#endif  // USART2_UART_INSTANCE

#ifndef USART3_UART_INSTANCE
#define USART3_UART_INSTANCE (false) //!< Indicates whether the USART3 peripheral instance is enabled
#endif  // USART3_UART_INSTANCE

#ifndef UART4_INSTANCE
#define UART4_INSTANCE (false) //!< Indicates whether the UART4 peripheral instance is enabled
#endif  // UART4_INSTANCE

#ifndef UART5_INSTANCE
#define UART5_INSTANCE (false) //!< Indicates whether the UART5 peripheral instance is enabled
#endif  // UART5_INSTANCE

#ifndef USART6_UART_INSTANCE
#define USART6_UART_INSTANCE (false) //!< Indicates whether the USART6 peripheral instance is enabled
#endif  // USART6_UART_INSTANCE

#ifndef UART7_INSTANCE
#define UART7_INSTANCE (false) //!< Indicates whether the UART7 peripheral instance is enabled
#endif  // UART7_INSTANCE

#ifndef UART8_INSTANCE
#define UART8_INSTANCE (false) //!< Indicates whether the UART8 peripheral instance is enabled
#endif  // UART8_INSTANCE

#endif  // STM32_CONFIG_HPP