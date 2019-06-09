project(routerd)

cmake_minimum_required(VERSION 3.10)

set(AC_COMMON_CXX_FLAGS "-std=c++17")
set(AC_TCMALLOC_CXX_FLAGS "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
set(AC_DEBUG_CXX_FLAGS "")
set(AC_TCMALLOC_LIBS "-ltcmalloc")

if("$ENV{IS_DEV}" STREQUAL "1")
    set(AC_TCMALLOC_CXX_FLAGS "")
    set(AC_DEBUG_CXX_FLAGS "-g")
    set(AC_TCMALLOC_LIBS "")

    set(CMAKE_EXE_LINKER_FLAGS "-g -Wl,-no_pie -fsanitize=address ${CMAKE_EXE_LINKER_FLAGS}")
    add_compile_options("-fsanitize=address")

else()
    add_compile_options("-O3")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${AC_COMMON_CXX_FLAGS} ${AC_TCMALLOC_CXX_FLAGS} ${AC_DEBUG_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

file(GLOB AC_ROUTERD_SOURCES *.cpp)

include_directories("../ac")
include_directories("../contrib")
include_directories(".")

add_subdirectory("routerd_lib")

add_executable(routerd ${AC_ROUTERD_SOURCES})

target_link_libraries(
    routerd
    routerd_lib
    ${AC_TCMALLOC_LIBS}
)

install(TARGETS routerd RUNTIME DESTINATION bin)
