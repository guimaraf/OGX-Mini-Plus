# OGX-Mini Plus - Compilation Guide

> **Project Version:** v1.1.1  
> **Last Updated:** 2026-01-29

This document contains all the information needed to compile the OGX-Mini Plus firmware.

---

## 📋 Requirements

### Required Tools

| Tool | Minimum Version | Download |
|------|-----------------|----------|
| **Git** | 2.x | [git-scm.com](https://git-scm.com/) |
| **Python** | 3.8+ | [python.org](https://www.python.org/) |
| **CMake** | 3.13+ | [cmake.org](https://cmake.org/) |
| **Ninja** | 1.10+ | [ninja-build.org](https://ninja-build.org/) |
| **ARM GCC Toolchain** | 14.x | [developer.arm.com](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) |

### Verify Installation

```powershell
git --version
python --version
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

---

## 📦 Dependencies

All dependencies are managed automatically via Git submodules:

| Library | Version | Notes |
|---------|---------|-------|
| **Pico SDK** | 2.1.1 | Downloaded by CMake (patched for GCC 14.x) |
| **TinyUSB** | 0.17.0 | ⚠️ Version 0.18.0+ is NOT compatible |
| **Bluepad32** | 4.1.0 | Fork with HID parser patches |
| **BTStack** | v1.6.1 | Fork with L2CAP security bypass |
| **Pico-PIO-USB** | latest | USB Host support |
| **libfixmath** | latest | Fixed-point math library |

---

## 🚀 Quick Start

### 1. Clone the Repository

```bash
git clone --recursive https://github.com/guimaraf/OGX-Mini-Plus.git
cd OGX-Mini-Plus
```

> **Note:** The `--recursive` flag automatically downloads all submodules.

### 2. Clear Environment Variable (if needed)

If you have `PICO_SDK_PATH` set in your system, clear it:

```powershell
$env:PICO_SDK_PATH = ""
```

### 3. Build

```powershell
cd Firmware/RP2040

# Pi Pico
cmake -S . -B build_pico -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO
cmake --build build_pico

# Pi Pico 2 W (Bluetooth)
cmake -S . -B build_pico2w -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO2W
cmake --build build_pico2w
```

---

## 🎮 Board Options (OGXM_BOARD)

| Value | Board | Features |
|-------|-------|----------|
| `PI_PICO` | Raspberry Pi Pico | USB Host |
| `PI_PICO2` | Raspberry Pi Pico 2 | USB Host, RP2350 |
| `PI_PICOW` | Raspberry Pi Pico W | Bluetooth |
| `PI_PICO2W` | Raspberry Pi Pico 2 W | Bluetooth, RP2350 |
| `RP2040_ZERO` | Waveshare RP2040-Zero | USB Host, RGB LED |
| `ADAFRUIT_FEATHER` | Adafruit Feather USB Host | USB Host, RGB LED |

---

## 💾 Installing Firmware

1. Hold the **BOOTSEL** button on the Pico
2. Connect the USB cable to your PC (keep holding BOOTSEL)
3. Release the button - a drive named **RPI-RP2** will appear
4. Copy the `.uf2` file to the drive
5. The Pico will automatically restart

---

## ⚙️ Build Options

### MAX_GAMEPADS

```bash
cmake -DMAX_GAMEPADS=4 ...
```

> With MAX_GAMEPADS > 1, only DInput (PS3) and Switch are supported.

### Debug Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ...
```

---

## ❗ Troubleshooting

### "Failed to initialize submodules"

```bash
git submodule update --init --recursive --force
```

### "`uint8_t` does not name a type" (pioasm)

This is already fixed in the forked pico-sdk. If using original SDK with GCC 14.x, add `#include <cstdint>` to:
- `pico-sdk/tools/pioasm/pio_types.h`
- `pico-sdk/tools/pioasm/output_format.h`

### "too few arguments to function usbd_edpt_xfer"

You are using TinyUSB 0.18.0+. The submodule should be at 0.17.0. Run:

```bash
cd Firmware/external/tinyusb
git checkout tags/0.17.0
cd ../../..
git submodule update --init --recursive
```

### "Using PICO_SDK_PATH from environment..."

Clear the environment variable:

```powershell
$env:PICO_SDK_PATH = ""
```

---

## 📁 Project Structure

```
OGX-Mini-Plus/
├── Firmware/
│   ├── RP2040/
│   │   ├── src/           # Source code
│   │   ├── cmake/         # Build scripts
│   │   ├── build_pico/    # Build output (Pi Pico)
│   │   └── build_pico2w/  # Build output (Pi Pico 2 W)
│   └── external/          # Submodules (auto-downloaded)
├── CompileHelp.md         # This file
├── DEPENDENCIES.md        # Detailed dependency info
└── README.md              # Project overview
```

---

## 🔗 See Also

- [DEPENDENCIES.md](DEPENDENCIES.md) - Detailed library versions and patches
- [README.md](README.md) - Project overview and usage
