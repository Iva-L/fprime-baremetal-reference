
####
# Per-chip-family OSAL backend dispatch. Add a new family's `elseif` block
# here (pointing at its own subdirectory) as support for it is added --
# each family's backend is mutually exclusive, selected by FPRIME_PLATFORM.
####

if (FPRIME_PLATFORM STREQUAL "stm32h7")
    add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Stm32H7")
endif()
