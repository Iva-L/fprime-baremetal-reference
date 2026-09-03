####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
#
####

register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/STM32Timer.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/STM32Timer.cpp"
    DEPENDS
        FprimeStm32
)
