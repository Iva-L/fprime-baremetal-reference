####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
#
####
register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriver.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriver.cpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriver.hpp"
  DEPENDS
    Os
    FprimeStm32
)

register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriver.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/Stm32UartDriverTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/Stm32UartDriverTester.cpp"
    DEPENDS
        STest # For rules-based testing
    UT_AUTO_HELPERS
)