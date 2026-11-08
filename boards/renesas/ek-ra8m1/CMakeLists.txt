# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Preempt Systems Oy
#
# KideRT on the Renesas EK-RA8M1 (R7FA8M1AH, Cortex-M85). The register
# definitions come from third_party/; the KideRT library and headers are
# copied into this folder (libkrt.a, krt/include) as the README describes.

cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../../arm-none-eabi.cmake"
    CACHE FILEPATH "Toolchain file")

project(ek-ra8m1 LANGUAGES C ASM)

set(CMAKE_C_STANDARD          11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The KideRT package for armv8.1-m.main is built without the FPU (soft-float
# ABI), so everything linked with it is too.
set(BOARD_CPU_FLAGS -march=armv8.1-m.main -mthumb -mfloat-abi=soft)
add_compile_options(${BOARD_CPU_FLAGS} -O2 -g -ffunction-sections -fdata-sections)
add_link_options(${BOARD_CPU_FLAGS})

add_executable(app.elf
    app/main.c
    board/board.c
    board/krt_bsp.c
    board/startup.c
    board/system.c)

target_include_directories(app.elf PRIVATE board)
target_include_directories(app.elf SYSTEM PRIVATE
    krt/include
    third_party/renesas/Include
    third_party/arm/cmsis/Core/Include)
target_compile_options(app.elf PRIVATE -Wall -Wextra)
target_link_libraries(app.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libkrt.a)
target_link_options(app.elf PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/board/ra8m1.ld
    -nostartfiles               # Reset_Handler is the entry, not crt0
    --specs=nano.specs
    -Wl,--gc-sections
    -Wl,-Map=app.map
    -Wl,--print-memory-usage)
set_target_properties(app.elf PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/board/ra8m1.ld)

add_custom_command(TARGET app.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex app.elf app.hex
    COMMAND ${CMAKE_SIZE_UTIL} app.elf)
