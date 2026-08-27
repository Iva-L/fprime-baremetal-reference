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
        "${CMAKE_CURRENT_LIST_DIR}/BootstrapAllocator.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/MallocWrappers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32h753xx.s"
    DEPENDS
        ${FPRIME_CURRENT_MODULE}_Top
        FprimeStm32
        Os_Baremetal_OverrideNewDelete
)

target_link_options(ReferenceDeployment PRIVATE
    "-T${CMAKE_CURRENT_LIST_DIR}/linker/STM32H753XIHx_FLASH.ld"
    "-Wl,--gc-sections"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/ReferenceDeployment.map"
    "-Wl,--undefined=_Znwj"
    "-Wl,--undefined=_Znaj"
    "-Wl,--undefined=_Znwjl"
    "-Wl,--undefined=_Znajl"
    "-Wl,--undefined=_ZnwjRKSt9nothrow_t"
    "-Wl,--undefined=_ZnajRKSt9nothrow_t"
    "-Wl,--undefined=_ZdlPv"
    "-Wl,--undefined=_ZdaPv"
    "-Wl,--undefined=_ZdlPvl"
    "-Wl,--undefined=_ZdaPvl"
    "-Wl,--undefined=_ZdlPvj"
    "-Wl,--undefined=_ZdaPvj"
    "-Wl,--wrap=malloc"
    "-Wl,--wrap=calloc"
    "-Wl,--wrap=realloc"
    "-Wl,--wrap=free"
    "--specs=nosys.specs"
)
target_sources(ReferenceDeployment PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32h753xx.s"
)
