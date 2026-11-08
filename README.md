# KideRT Boards

Board support and example applications for the KideRT real-time operating
system on evaluation boards. Each board comes with an application that links
against KideRT and runs on the board as it ships.

KideRT itself is not part of this repository. Every application links against
the kernel from a KideRT package you get separately, and every board here
builds with the KideRT evaluation package. The BSP is not taken from the
package: each board's `board/` folder is the BSP KideRT runs on. To get a
package, contact Preempt Systems Oy at contact@preemptsystems.com.

## Boards

| Board | Core | Architecture | Application |
|-------|------|-------------|-------------|
| [Renesas EK-RA8M1](boards/renesas/ek-ra8m1) | Cortex-M85 | `armv8.1-m.main` | LED blink, console echo |

Boards are added to this table as they are brought up.

## Getting started

You need the [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
(`arm-none-eabi-gcc`), [CMake](https://cmake.org) 3.20 or later and
[Ninja](https://ninja-build.org).

1. Get KideRT for your board.

   With the evaluation package, unpack it:

   ```bash
   tar xzf krt-<version>-<arch>-eval.tar.gz
   ```

   With a licensed package, set it up for the board's core as its README
   describes, then build it:

   ```bash
   cd /path/to/krt
   cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi-gcc.cmake
   ninja -C build
   ```

2. Go to the board's folder and copy in the kernel library and headers. `KRT`
   is the package folder from step 1:

   ```bash
   cd boards/<vendor>/<board>
   KRT=/path/to/krt-<version>-<arch>-eval
   mkdir -p krt
   cp -R $KRT/krt/include krt/
   cp $KRT/krt/libkrt-*.a libkrt.a
   ```

   With a licensed package, the library is `$KRT/build/krt/libkrt.a`.

3. Build the application from the board's folder:

   ```bash
   cmake -B build -G Ninja
   ninja -C build
   ```

4. Flash `build/app.elf` as the board's README describes.

## Layout

```
boards/
└── <vendor>/
    └── <board>/
        ├── README.md     the kit, how to flash it and what it prints
        ├── CMakeLists.txt
        ├── board/        the BSP: startup, clocks, pins, console, LEDs, linker script
        ├── app/          the application
        ├── libkrt.a      copied from the KideRT package
        └── krt/include/  copied from the KideRT package
third_party/              unmodified vendor files, e.g. device headers
arm-none-eabi.cmake       toolchain file
```

## License

The contents of this repository are licensed under the Apache License 2.0; see
[LICENSE](LICENSE). Vendor files keep the license stated in their headers, and
each folder under `third_party/` carries its vendor's license file. KideRT is
licensed separately under its own terms.
