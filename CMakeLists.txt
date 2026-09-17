####
# This sets up the build system for the 'FprimeBaremetalReference' project, including
# components and deployments from project.cmake. In addition, it imports the core F Prime components.
####

cmake_minimum_required(VERSION 3.24.2)
project(FprimeBaremetalReference C CXX)
enable_language(ASM)

###
# F' Core Setup
# This includes all of the F prime core components, and imports the make-system.
###
include("${CMAKE_CURRENT_LIST_DIR}/lib/fprime/cmake/FPrime.cmake")
# NOTE: register custom targets between these two lines
fprime_setup_included_code()

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-stm32")

# fprime-sensors: register only the MpuImu pieces this project actually uses.
include_directories("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-sensors" "${CMAKE_CURRENT_BINARY_DIR}/lib/fprime-sensors")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-sensors/fprime-sensors/Helpers")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-sensors/fprime-sensors/MpuImu/Types")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-sensors/fprime-sensors/MpuImu/Ports")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-sensors/fprime-sensors/MpuImu/Components")

# Register project-level configuration overrides after framework and library
# configuration modules have been added.
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/config")

# This includes project-wide objects
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FprimeBaremetalReference")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FprimeBaremetalReference/Deployments/ReferenceDeployment/")
