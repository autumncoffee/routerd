file(GLOB AC_ROUTERD_HANDLERS_SOURCES *.cpp)

add_library(routerd_handlers STATIC ${AC_ROUTERD_HANDLERS_SOURCES})

target_link_libraries(
    routerd_handlers
    routerd_lib
    ac_common
)
