# EK-RA8M1

KideRT on the Renesas [EK-RA8M1](https://www.renesas.com/en/design-resources/boards-kits/ek-ra8m1)
evaluation kit: R7FA8M1AH, Arm Cortex-M85 at 480 MHz. The board code is
written against Renesas's register definitions
([third_party/renesas](../../../third_party/renesas)) and needs no other
Renesas software. No Renesas tool is needed to build or flash.

The application runs two threads:

- the blue LED (LED1) blinks once a second;
- the console prints a banner and echoes what you type, and the green LED
  (LED2) toggles on each character. The console's thread is woken by the
  UART receive interrupt through `krt_thread_notify()`.

## What you need

- The Arm GNU Toolchain, CMake 3.20 or later and Ninja, as in the
  [top-level README](../../../README.md).
- The KideRT evaluation package for `armv8.1-m.main`,
  `krt-<version>-armv8.1-m.main-eval.tar.gz`.
- [SEGGER J-Link Software](https://www.segger.com/downloads/jlink/) to flash
  the board through its on-board J-Link.

## Build

1. Unpack the evaluation package:

   ```bash
   tar xzf krt-<version>-armv8.1-m.main-eval.tar.gz
   ```

2. Go to this board's folder and copy in the kernel library and headers.
   `KRT` is the unpacked package folder:

   ```bash
   cd boards/renesas/ek-ra8m1
   KRT=/path/to/krt-<version>-armv8.1-m.main-eval
   mkdir -p krt
   cp -R $KRT/krt/include krt/
   cp $KRT/krt/libkrt-*.a libkrt.a
   ```

   With a licensed package, the library is `$KRT/build/krt/libkrt.a`.

3. Build from the board's folder:

   ```bash
   cmake -B build -G Ninja
   ninja -C build
   ```

This produces `build/app.elf` and `build/app.hex`.

## Flash and run

1. Connect the USB port of the kit's on-board J-Link to your computer. Leave
   the J-Link OB jumpers in their default positions (see the kit's user
   manual).
2. From the board's folder, flash and start the image:

   ```bash
   JLinkExe -device R7FA8M1AH -if SWD -speed 4000 -autoconnect 1 -CommanderScript flash.jlink
   ```

3. Open the J-Link virtual COM port at 115200 baud, 8N1, and press the reset
   button. The board prints:

   ```
   KideRT on EK-RA8M1 (R7FA8M1AH, Cortex-M85)
   Type and the board echoes it back.
   ```

The image is a flat one: no TrustZone partition, everything runs in the
secure world. Flashing writes code flash only; the option-setting memory
(watchdog start, TrustZone boundaries, boot settings) is left as it is.

## How it is put together

```
board/
├── startup.c        reset entry and vector table
├── system.c         caches, C runtime, clocks, I/O voltage, security attribution
├── board.c          pins, LEDs and the SCI9 console; krt_bsp_init()
├── board.h          LEDs and console, for the application
├── krt_bsp.c        System Timer on SysTick, and the kernel's fault hook
├── krt_bsp.h        the KideRT BSP interface
├── board_private.h  the device header plus declarations shared by the files above
└── ra8m1.ld         linker script, with the kernel's heap and thread stack pool
app/main.c           the application
libkrt.a             copied from the KideRT package
krt/include/         copied from the KideRT package
```

- **BSP.** `krt_bsp_init()` sets up the board; `krt_bsp_systickreload()`,
  `krt_bsp_systickgetcount()` and `KRT_BSP_SYSTICK_IRQN` drive the kernel's
  System Timer from SysTick; `bsp_faulthook()`, which `krt.h` declares, is
  the kernel's fault hook.
- **Clocks.** `SystemInit` brings the clocks up before `main()`: 20 MHz
  crystal → PLL1 960 MHz → 480 MHz core clock, ICLK 240 MHz, PCLKA 120 MHz,
  and SCICLK 120 MHz for the console. SysTick runs from the core clock and
  belongs to KideRT.
- **Vector table.** KideRT's SVC, PendSV, SysTick and fault handlers are plain
  references with no weak default.
- **Console.** SCI9 on P1014/P1015 at 115200 baud. Sending polls the 16-byte
  transmit FIFO; receiving is interrupt-driven.
- **Caches.** Instruction cache and branch prediction on, data cache off: the
  MPU belongs to KideRT.

## License

`third_party/renesas` is BSD-3-Clause and `third_party/arm/cmsis` is
Apache-2.0; each folder carries its license. If you hand out a binary built
from this board, include Renesas's
[LICENSE.md](../../../third_party/renesas/LICENSE.md) with it. This folder is
Apache-2.0.
