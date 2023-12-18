cmake_minimum_required(VERSION 3.27)
project(lwFW C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig)
pkg_search_module(LIBDPDK REQUIRED libdpdk)

include_directories(src)

add_executable(lwFW
        src/main.c
        src/stats.c
        src/stats.h
        src/dpdk.h
        src/ports.h
        src/ports.c
        src/worker.c
        src/worker.h)

target_compile_options(lwFW PRIVATE ${LIBDPDK_STATIC_CFLAGS})
target_link_libraries(lwFW ${LIBDPDK_STATIC_LDFLAGS} -pthread)

