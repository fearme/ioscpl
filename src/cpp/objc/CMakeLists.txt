
if(${MARIO_PLATFORM} STREQUAL "ios")

  add_library(mwpobjcinterop mwp_ios_interop.mm)

  target_link_libraries(mwpobjcinterop
                        "-framework UIKit"
                        "-framework SystemConfiguration")

endif()

