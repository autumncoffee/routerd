file(GLOB AC_ROUTERD_LIB_SOURCES *.cpp)

include_directories("../../ac")
include_directories("../../contrib")
include_directories("..")

add_subdirectory("handlers")

add_subdirectory("../../ac/ac-common" ac_common_bindir)
add_subdirectory("../../ac/ac-library/http" ac_library_http_bindir)

add_library(routerd_lib STATIC ${AC_ROUTERD_LIB_SOURCES})

target_link_libraries(
    routerd_lib
    routerd_handlers
    ac_common
    ac_library_http
    ac_library_http_router
    "-lpcrecpp"
)
