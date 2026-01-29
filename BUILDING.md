# OGX-Mini Plus - Build Instructions

> **Project Version:** v1.1.1  
> **Last Updated:** 2026-01-29

---

## Requirements

### Required Tools

| Tool | Minimum Version | Link |
|------|-----------------|------|
| Git | 2.x | [git-scm.com](https://git-scm.com/) |
| Python | 3.8+ | [python.org](https://www.python.org/) |
| CMake | 3.13+ | [cmake.org](https://cmake.org/) |
| Ninja | 1.10+ | [ninja-build.org](https://ninja-build.org/) |
| ARM GCC Toolchain | 14.x | [developer.arm.com](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) |

### Verify Installation

```powershell
git --version
python --version
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

---

## Cloning the Repository

```bash
git clone --recursive https://github.com/guimaraf/OGX-Mini-Plus.git
cd OGX-Mini-Plus
```

> **Note:** The `--recursive` flag automatically downloads all submodules (bluepad32, tinyusb, etc.)

If you already cloned without `--recursive`:
```bash
git submodule update --init --recursive
```

---

## Building

### Board Options (OGXM_BOARD)

| Value | Board | Features |
|-------|-------|----------|
| `PI_PICO` | Raspberry Pi Pico | USB Host |
| `PI_PICO2` | Raspberry Pi Pico 2 | USB Host, RP2350 |
| `PI_PICOW` | Raspberry Pi Pico W | Bluetooth |
| `PI_PICO2W` | Raspberry Pi Pico 2 W | Bluetooth, RP2350 |
| `RP2040_ZERO` | Waveshare RP2040-Zero | USB Host, RGB LED |
| `ADAFRUIT_FEATHER` | Adafruit Feather USB Host | USB Host, RGB LED |

---

### Build for Pi Pico (RP2040)

```powershell
cd Firmware/RP2040

# Configure
cmake -S . -B build_pico -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO

# Build
cmake --build build_pico
```

**Output file:** `build_pico/OGX-Mini-v1.1.1-PI_PICO.uf2`

---

### Build for Pi Pico 2 W (RP2350 + Bluetooth)

```powershell
cd Firmware/RP2040

# Configure
cmake -S . -B build_pico2w -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO2W

# Build
cmake --build build_pico2w
```

**Output file:** `build_pico2w/OGX-Mini-v1.1.1-PI_PICO2W.uf2`

---

## Installing the Firmware

1. Hold the **BOOTSEL** button on the Pico
2. Connect the USB cable to your PC (keep holding BOOTSEL)
3. Release the button - a drive named **RPI-RP2** will appear
4. Copy the `.uf2` file to the drive
5. The Pico will automatically restart

---

## Troubleshooting

### Error: "Failed to initialize submodules"

```bash
git submodule update --init --recursive --force
```

### Error: "`uint8_t` does not name a type" (pioasm)

This is fixed in the forked pico-sdk. If using original SDK with GCC 14.x, add `#include <cstdint>` to:
- `pico-sdk/tools/pioasm/pio_types.h`
- `pico-sdk/tools/pioasm/output_format.h`

### Error: "too few arguments to function usbd_edpt_xfer"

You are using TinyUSB 0.18.0+. Checkout version 0.17.0:

```bash
cd Firmware/external/tinyusb
git checkout tags/0.17.0
```

---

## Additional Settings

### MAX_GAMEPADS

By default, the firmware supports 1 controller. To support up to 4:

```bash
cmake -DMAX_GAMEPADS=4 ...
```

> **Note:** With MAX_GAMEPADS > 1, only DInput (PS3) and Switch are supported.

### Debug Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ...
```

---

## See Also

- [DEPENDENCIES.md](DEPENDENCIES.md) - Library list and versions
- [README.md](README.md) - Project overview

---

## ⚠️ Important Notice for Developers

### Files that should NOT be committed:

The `Firmware/external/` folder is **automatically managed** by the Git submodule system and CMake scripts. It contains:
- `bluepad32/` - Downloaded via submodule
- `tinyusb/` - Downloaded via submodule
- `pico-sdk/` - Downloaded by CMake during configuration
- `Pico-PIO-USB/` - Downloaded via submodule
- `libfixmath/` - Downloaded via submodule

**DO NOT add this folder to Git!** It is listed in `.gitignore` and should remain that way.

### Build directories:

The `build_pico/` and `build_pico2w/` folders contain compiled files and should also **NOT be committed**.
