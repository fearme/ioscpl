
if(${MARIO_PLATFORM} STREQUAL "ios")

  add_library(mwpobjcinterop mwp_ios_interop.mm)

  target_link_libraries(mwpobjcinterop
                        "-framework UIKit"
                        "-framework SystemConfiguration")

elseif(${MARIO_PLATFORM} STREQUAL "macosx")

  add_library(mwpobjcinterop mwp_osx_interop.mm)

  target_link_libraries(mwpobjcinterop
                        "-framework AppKit"
                        "-framework SystemConfiguration")

endif()

