# EK-RA8M1

KideRT on the Renesas [EK-RA8M1](https://www.renesas.com/en/design-resources/boards-kits/ek-ra8m1):
R7FA8M1AH, Cortex-M85 at 480 MHz. No Renesas software or tools are needed;
the board code uses only the register definitions in
[third_party/renesas](third_party/renesas).

The application blinks the blue LED (LED1) once a second, fades the green LED
(LED2) in and out with 1 kHz software PWM timed by `nanosleep()`, and echoes
what you type on the console.

## What you need

- Arm GNU Toolchain, CMake 3.20 or later and Ninja
- KideRT evaluation package for `armv8.1-m.main`, tested with v1.3.0
- [SEGGER J-Link Software](https://www.segger.com/downloads/jlink/)

## Build

1. Unpack the evaluation package:

   ```bash
   tar xzf KideRT-<version>-armv8.1-m.main-eval.tar.gz
   ```

2. Copy the kernel library and headers into this folder. `KRT` is the
   unpacked package:

   ```bash
   cd boards/renesas/ek-ra8m1
   KRT=/path/to/KideRT-<version>-armv8.1-m.main-eval
   mkdir -p krt
   cp -R $KRT/krt/include krt/
   cp $KRT/krt/libkrt-*.a libkrt.a
   ```

   With a licensed package, the library is `$KRT/build/krt/libkrt.a`.

3. Build:

   ```bash
   cmake -B build -G Ninja
   ninja -C build
   ```

## Flash and run

1. Connect the J-Link USB port (J10) and leave the jumpers at their defaults.
2. Flash from this folder:

   ```bash
   JLinkExe -device R7FA8M1AH -if SWD -speed 4000 -autoconnect 1 -CommanderScript flash.jlink
   ```

3. Open the J-Link virtual COM port at 115200 baud, 8N1, and press reset.
   KideRT prints its boot banner and boot log, then the application prints
   the last two lines:

   ```
   888    d8P  d8b      888          8888888b.  88888888888
   888   d8P   Y8P      888          888   Y88b     888
   888  d8P             888          888    888     888
   888d88K     888  .d88888  .d88b.  888   d88P     888
   8888888b    888 d88" 888 d8P  Y8b 8888888P"      888
   888  Y88b   888 888  888 88888888 888 T88b       888
   888   Y88b  888 Y88b 888 Y8b.     888  T88b      888
   888    Y88b 888  "Y88888  "Y8888  888   T88b     888

   POSIX Real-Time Operating System for Arm Cortex-M

   v1.3.0 (2026-11-14)
   Copyright (c) Preempt Systems Oy

   Booting...
   Initializing CPU core
   Initializing memory
     Kernel heap: 8192 B
     Thread stack pool: 4096 B
   Initializing threading subsystem
     Initializing scheduler
     Creating idle thread
     Creating startup thread
     Creating timer thread
   Initializing Thread-Local Storage support
   Configuring thread stack
   Starting System Timer
     Programming initial timer period of 4800000

   Boot sequence complete
   Dispatching startup thread at 0x2000049

   KideRT on EK-RA8M1 (R7FA8M1AH, Cortex-M85)
   Type and the board echoes it back.
   ```

The image runs entirely in the secure world. Flashing leaves the
option-setting memory untouched.

## Layout

```
board/
├── startup.c        reset entry and vector table
├── system.c         C runtime, clocks, I/O voltage, security attribution
├── board.c          pins, LEDs, SCI9 console, krt_bsp_init()
├── board.h          LED and console API for the application
├── krt_bsp.c/.h     KideRT BSP: System Timer on SysTick, fault hook, console
├── board_private.h  device header and shared declarations
└── ra8m1.ld         linker script
app/main.c           the application
third_party/         Renesas and Arm CMSIS headers, unmodified
```

Clocks: 20 MHz crystal → PLL1 960 MHz → 480 MHz CPU, 120 MHz SCICLK.
Console: SCI9 on P1014 (TX) and P1015 (RX).

## License

`third_party/renesas` is BSD-3-Clause and `third_party/arm/cmsis` is
Apache-2.0. Ship Renesas's [LICENSE.md](third_party/renesas/LICENSE.md) with
any binary built from this board. Everything else here is Apache-2.0.
