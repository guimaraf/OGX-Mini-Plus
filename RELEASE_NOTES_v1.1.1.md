# OGX-Mini - Fred Version 1.1.1

## 🎮 Features new to v1.1.1 (by Fred)

### New PlayStation 4 (DS4) Driver
- Added a new driver that emulates the authentic **Sony DualShock 4** (VID: 054C, PID: 09CC).
- This driver is **natively recognized by the PlayStation 3**, providing proper analog triggers (L2/R2) and Select button functionality.
- ⚠️ **PS3 Limitation**: The **Home button is not recognized** on PlayStation 3, but all other functions work correctly.
- **Nintendo Wii Support**: The DS4 driver allows using the OGX-Mini on **Nintendo Wii with FakeMote enabled**.
- **OPL Compatibility**: May also work on **Open PS2 Loader (OPL)**, but this has not been tested as I do not have a PS2 available.
- **Activation**: `Start + Dpad Left + A`

### New 360 Guitar Driver (Native)
- Added a native **Xbox 360 guitar driver** (VID: 045E, PID: 028E).
- Selectable via `Start + Dpad Up + A`.
- Allows the device to be recognized specifically as a guitar by the Xbox 360.
- **All Xbox 360 games that support guitars** (Rock Band, Guitar Hero, etc.) are now playable with this driver!
- ⚠️ **Tested only** with the **official PlayStation 3 Guitar Hero 3 guitar**. Other guitars have not been tested.
- ⚠️ **Start Power Limitation**: The **Star Power button could not be emulated directly**. However, pressing **Select activates Star Power** as a workaround.

### PS3 Driver Fixes
- Fixed a bug that **prevented playing Guitar Hero 3 and Gran Turismo 6**.
- Fixed an issue where pressing the **Home/Guide button** on the controller would trigger multiple phantom inputs. The button now functions correctly.

## 🔧 Build Information

This version (v1.1.1) was compiled and tested for:
- **Pi Pico** (RP2040)
- **Pi Pico 2 W** (RP2350 with Bluetooth)

These are the only boards I currently have available for testing.

### Included Firmware Files

| Board | Firmware File |
|-------|---------------|
| Pi Pico | `OGX-Mini-v1.1.1-fred-PI_PICO.uf2` |
| Pi Pico 2 W | `OGX-Mini-v1.1.1-fred-PI_PICO2W.uf2` |

---

## ⚠️ Known Issues

The issue with phantom inputs has not yet been resolved. It occurs in some games such as:
- **God of War 3**: D-pad left and right analog stick up
- **GTA 5**: D-pad up

---

## 📋 How to Flash

1. Hold the **BOOTSEL** button on your Pi Pico while connecting it to your PC via USB.
2. The Pico will appear as a USB storage device.
3. Drag and drop the corresponding `.uf2` file to the device.
4. The Pico will automatically reboot with the new firmware.

---

## 🎮 Changing Platforms

By default the OGX-Mini will emulate an OG Xbox controller. Hold a button combo for 3 seconds to change platforms:

| Platform | Button Combo |
|----------|--------------|
| XInput | Start + Dpad Up |
| 360 Guitar (Native) | Start + Dpad Up + A |
| Original Xbox | Start + Dpad Right |
| Original Xbox Steel Battalion | Start + Dpad Right + RB |
| Original Xbox DVD Remote | Start + Dpad Right + LB |
| Nintendo Switch | Start + Dpad Down |
| PlayStation 3 | Start + Dpad Left |
| PlayStation 4 (DS4) | Start + Dpad Left + A |
| PlayStation Classic | Start + A |
| Web Application Mode | Start + LB + RB |

Your chosen mode will persist after powering off the device.

---

## 🔗 Links

- [Web App for Configuration](https://wiredopposite.github.io/OGX-Mini-WebApp/)
- [Original Project Repository](https://github.com/wiredopposite/OGX-Mini)