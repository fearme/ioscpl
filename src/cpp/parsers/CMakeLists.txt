
RAGEL_TARGET(uri mwp_uri.rl ${CMAKE_CURRENT_BINARY_DIR}/mwp_uri.cpp)

add_library(mwpparsers
                ${RAGEL_uri_OUTPUTS}
                )

