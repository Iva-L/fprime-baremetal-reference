#####
# 'ReferenceDeployment' Deployment:
#
# This registers the 'ReferenceDeployment' deployment to the build system. 
# Custom components that have not been added at the project-level should be added to 
# the list below.
#
#####

###
# Topology and Components
###

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top/")

# Add custom components to this specific deployment here
# add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/MyComponent/")

register_fprime_deployment(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32h753xx.s"
    DEPENDS
        ${FPRIME_CURRENT_MODULE}_Top
        FprimeStm32
)

target_link_options(ReferenceDeployment PRIVATE
    "-T${CMAKE_CURRENT_LIST_DIR}/linker/STM32H753XIHx_FLASH.ld"
    "-Wl,--gc-sections"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/ReferenceDeployment.map"
    "--specs=nosys.specs"
)
target_sources(ReferenceDeployment PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32h753xx.s"
)
