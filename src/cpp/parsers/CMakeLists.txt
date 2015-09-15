
RAGEL_TARGET(uri mwp_uri.rl ${CMAKE_CURRENT_BINARY_DIR}/mwp_uri.cpp)
RAGEL_TARGET(args mwp_args.rl ${CMAKE_CURRENT_BINARY_DIR}/mwp_args.cpp)

add_library(mwpparsers
                ${RAGEL_uri_OUTPUTS}
                ${RAGEL_args_OUTPUTS}
                )

