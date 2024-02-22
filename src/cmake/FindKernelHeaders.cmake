# Finding kernel headers
set(KERNEL_HEADERS_DIR CACHE STRING "Path to the kernel headers directory")
if("${KERNEL_HEADERS_DIR}" STREQUAL "")
    execute_process(COMMAND uname -r OUTPUT_VARIABLE KERNEL_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(KERNEL_HEADERS_DIR "/usr/src/linux-headers-${KERNEL_RELEASE}/")
endif()
find_file(kernel_makefile NAMES Makefile PATHS ${KERNEL_HEADERS_DIR} NO_DEFAULT_PATH)
if(NOT kernel_makefile)
    message(FATAL_ERROR "Kernel headers not found!")
endif()
message(STATUS "Using kernel ${KERNEL_RELEASE}")
set(KERNEL_HEADERS_INCLUDE_DIRS
        ${KERNEL_HEADERS_DIR}/include
        ${KERNEL_HEADERS_DIR}/arch/x86/include
        CACHE PATH "Kernel headers include dirs"
)
