####
# Project configuration
#
# Keep the framework defaults as the base configuration and layer the
# bare-metal library configuration on top of them.
####

# Project-level configuration overrides must be processed after the framework
# and bare-metal base configurations so they replace the default files.
register_fprime_config(
    ReferenceDeploymentConfig
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/FpConfig.h"
        "${CMAKE_CURRENT_LIST_DIR}/FpConstants.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/AcConstants.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/ComCcsdsConfig.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/DataProductsConfig.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/TlmChanImplCfg.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/CommandDispatcherImplCfg.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/PrmDbImplCfg.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/DpCatalogCfg.hpp"
    INTERFACE
)
