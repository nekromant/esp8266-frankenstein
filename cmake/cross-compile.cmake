# This file assumes that you have a linaro abe-based toolchain
# with a raspbian sysroot somewhere inside. This file also
# takes care to trick pkg-config into searching only toolchain's sysroot for
# the libraries

if (NOT CMAKE_LIBRARY_PATH)
    SET(CMAKE_LIBRARY_PATH ${CROSS_COMPILE})
endif()

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_C_COMPILER_WORKS YES)
SET(CMAKE_CXX_COMPILER_WORKS YES)

if (CMAKE_CLANG_CROSS)
    set(CMAKE_C_COMPILER clang)
    set(CROSS_COMMON_FLAGS -target ${CROSS_COMPILE} -ccc-gcc-name ${CROSS_COMPILE}-gcc)
endif()

if (NOT CMAKE_C_COMPILER)
    SET(CMAKE_C_COMPILER     ${CROSS_COMPILE}-gcc${CMAKE_EXECUTABLE_SUFFIX})
endif()

if (NOT CMAKE_C_COMPILER)
    SET(CMAKE_CXX_COMPILER   ${CROSS_COMPILE}-g++${CMAKE_EXECUTABLE_SUFFIX})
endif()

# where is the target environment
# This macro should be called once AFTER PROJECT() directive
macro(CROSS_COMPILE_DETECT_SYSROOT)
    find_program(CROSS_TOOLCHAIN_PATH NAMES ${CMAKE_C_COMPILER})
    get_filename_component(CROSS_TOOLCHAIN_PATH "${CROSS_TOOLCHAIN_PATH}" PATH)

    if (EXISTS ${CROSS_TOOLCHAIN_PATH}/../${CROSS_COMPILE}/sysroot)
        SET(CMAKE_FIND_ROOT_PATH  ${CROSS_TOOLCHAIN_PATH}/../${CROSS_COMPILE}/sysroot)
    elseif(EXISTS ${CROSS_TOOLCHAIN_PATH}/../${CROSS_COMPILE}/libc)
        SET(CMAKE_FIND_ROOT_PATH  ${CROSS_TOOLCHAIN_PATH}/../${CROSS_COMPILE}/libc)
    else()
        message(WARNING "Couldn't auto-detect sysroot dir")
    endif()

    # search for programs in the build host directories
    SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    # for libraries and headers in the target directories
    SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endmacro()
