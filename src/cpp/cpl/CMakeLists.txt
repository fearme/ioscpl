
# Build the CPL interface file
set(MWP_INTERFACE_NAME cpl)
set(MWP_INTERFACE_NAME_UC CPL)

configure_file(
  "${PROJECT_SOURCE_DIR}/core/hp_mwp.h.in"
  "${PROJECT_BINARY_DIR}/cpl/hp_${MWP_INTERFACE_NAME}.h"
  )

include_directories("${PROJECT_BINARY_DIR}/cpl")

# Make libcpl
add_library(cpl
                     hp_cpl.cpp
                     cpl_controlled_print_api.cpp)


