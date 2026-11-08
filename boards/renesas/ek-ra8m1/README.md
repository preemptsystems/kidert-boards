# EK-RA8M1

KideRT on the Renesas [EK-RA8M1](https://www.renesas.com/en/design-resources/boards-kits/ek-ra8m1):
R7FA8M1AH, Cortex-M85 at 480 MHz. No Renesas software or tools are needed;
the board code uses only the register definitions in
[third_party/renesas](third_party/renesas).

The application blinks the blue LED (LED1) once a second and echoes what you
type on the console, toggling the green LED (LED2) on each character.

## What you need

- Arm GNU Toolchain, CMake 3.20 or later and Ninja
- KideRT evaluation package for `armv8.1-m.main`
- [SEGGER J-Link Software](https://www.segger.com/downloads/jlink/)

## Build

From this folder, with `KRT` set to the unpacked package:

```bash
mkdir -p krt
cp -R $KRT/krt/include krt/
cp $KRT/krt/libkrt-*.a libkrt.a
cmake -B build -G Ninja
ninja -C build
```

With a licensed package, the library is `$KRT/build/krt/libkrt.a`.

## Flash and run

1. Connect the J-Link USB port (J10) and leave the jumpers at their defaults.
2. Flash from this folder:

   ```bash
   JLinkExe -device R7FA8M1AH -if SWD -speed 4000 -autoconnect 1 -CommanderScript flash.jlink
   ```

3. Open the J-Link virtual COM port at 115200 baud, 8N1, and press reset:

   ```
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
├── krt_bsp.c/.h     KideRT BSP: System Timer on SysTick, fault hook
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
