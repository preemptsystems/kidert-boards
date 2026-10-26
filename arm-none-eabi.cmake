# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Preempt Systems Oy
#
# CMake toolchain file for the Arm GNU Toolchain (arm-none-eabi-gcc).

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(CMAKE_OBJCOPY   arm-none-eabi-objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL arm-none-eabi-size    CACHE INTERNAL "size tool")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
