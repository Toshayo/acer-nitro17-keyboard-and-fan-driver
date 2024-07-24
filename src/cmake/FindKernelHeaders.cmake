# Finding kernel headers
set(KERNEL_HEADERS_DIR CACHE STRING "Path to the kernel headers directory")

# Accept the version argument from find_package
if(KernelHeaders_FIND_VERSION)
    set(KERNEL_VERSION "${KernelHeaders_FIND_VERSION}")
    file(GLOB KERNEL_HEADERS_DIRS "/usr/src/linux-headers-${KERNEL_VERSION}-*")
else()
    # Determine the kernel version to use
    execute_process(COMMAND uname -r OUTPUT_VARIABLE KERNEL_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(KERNEL_HEADERS_DIRS "/usr/src/linux-headers-${KERNEL_VERSION}")
endif()

if("${KERNEL_HEADERS_DIR}" STREQUAL "")

    if(NOT KERNEL_HEADERS_DIRS)
        message(FATAL_ERROR "No kernel headers directories found for version ${KERNEL_VERSION}!")
    endif()

    # Sort the list to get the last item
    list(SORT KERNEL_HEADERS_DIRS)

    # Get the last item in the list
    list(GET KERNEL_HEADERS_DIRS -1 KERNEL_HEADERS_DIR)
endif()

set(KERNEL_MAKEFILE_PATH "${KERNEL_HEADERS_DIR}/Makefile")
if(NOT EXISTS "${KERNEL_MAKEFILE_PATH}")
    message(FATAL_ERROR "Kernel headers not found!")
endif()

message(STATUS "Using kernel ${KERNEL_VERSION} (${KERNEL_HEADERS_DIR})")

set(KERNEL_HEADERS_INCLUDE_DIRS
        ${KERNEL_HEADERS_DIR}/include
        ${KERNEL_HEADERS_DIR}/arch/x86/include
        CACHE PATH "Kernel headers include dirs"
)
