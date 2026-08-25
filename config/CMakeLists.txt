####
# Project configuration
#
# Keep the framework defaults as the base configuration and layer the
# bare-metal library configuration on top of them.
####

add_fprime_subdirectory("${FPRIME_FRAMEWORK_PATH}/default/config")
add_fprime_subdirectory("${FPRIME_PROJECT_ROOT}/lib/fprime-baremetal/fprime-baremetal/config")
