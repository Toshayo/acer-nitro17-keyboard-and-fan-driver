cmake_minimum_required(VERSION 3.26)
project(acer-nitro17-driver C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(KernelHeaders REQUIRED)

# Adding kernel headers includes
include_directories(${KERNEL_HEADERS_INCLUDE_DIRS})

# Required for CLion
add_definitions(-D__KERNEL__ -DMODULE)
add_library(dummy_target acer-nitro17.c)

# Building
set(DRIVER_FILE acer-nitro17.ko)
set(KBUILD_CMD ${CMAKE_MAKE_PROGRAM} -C ${KERNEL_HEADERS_DIR} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})

# Generate the Kbuild file through cmake.
FILE(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Kbuild "obj-m := acer-nitro17.o")

add_custom_command(
        OUTPUT ${DRIVER_FILE}
        COMMAND ${KBUILD_CMD} modules
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS acer-nitro17.c VERBATIM
)

add_custom_target(build ALL DEPENDS ${DRIVER_FILE})
add_custom_target(build-clean COMMAND ${KBUILD_CMD} clean)
