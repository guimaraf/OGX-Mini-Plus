# OGX-Mini

v1.1.1

![alt text](images/OGX-Mini-github.jpg)

Firmware for the RP2040, capable of emulating gamepads for several game consoles. The firmware comes in many flavors, supported on the Adafruit Feather USB Host board, Pi Pico, Pi Pico 2, Pi Pico W, Pi Pico 2 W, Waveshare RP2040-Zero, Pico/ESP32 hybrid, and a 4-Channel RP2040-Zero setup.

[**Visit the web app here**](https://wiredopposite.github.io/OGX-Mini-WebApp/) to change your mappings and deadzone settings. To pair the OGX-Mini with the web app via USB, plug your controller in, then connect it to your PC, hold Start + Left Bumper + Right Bumper to enter web app mode. Click "Connect via USB" in the web app and select the OGX-Mini. You can also pair via Bluetooth, no extra steps are needed in that case.

## Supported platforms

- Original Xbox
- Playstation 3
- Playstation 4 (DS4)
- Nintendo Switch (docked)
- XInput (use [**UsbdSecPatch**](https://github.com/InvoxiPlayGames/UsbdSecPatch) for Xbox 360, or select the patch in J-Runner while flashing your NAND)
- Playstation Classic
- DInput

## Changing platforms

By default the OGX-Mini will emulate an OG Xbox controller, you must hold a button combo for 3 seconds to change which platform you want to play on. Your chosen mode will persist after powering off the device.

Start = Plus (Switch) = Options (Dualsense/DS4)

- XInput
  - Start + Dpad Up
- 360 Guitar (Native Xbox 360 Guitar) Hamonix Rock Band Guitar
  - Start + Dpad Up + A
- Original Xbox
  - Start + Dpad Right
- Original Xbox Steel Battalion
  - Start + Dpad Right + Right Bumper
- Original Xbox DVD Remote
  - Start + Dpad Right + Left Bumper
- Switch
  - Start + Dpad Down
- PlayStation 3
  - Start + Dpad Left
- PlayStation 4 (DS4)
  - Start + Dpad Left + A
- PlayStation Classic
  - Start + A (Cross for PlayStation and B for Switch gamepads)
- Web Application Mode
  - Start + Left Bumper + Right Bumper

After a new mode is stored, the RP2040 will reset itself so you don't need to unplug it.

## Supported devices
### Wired controllers
- Original Xbox Duke and S
- Xbox 360, One, Series, and Elite
- Dualshock 3 (PS3)
- Dualshock 4 (PS4)
- Dualsense (PS5)
- Nintendo Switch Pro
- Nintendo Switch wired
- Nintendo 64 Generic USB
- Playstation Classic
- Generic DInput
- Generic HID (mappings may need to be editted in the web app)

Note: There are some third party controllers that can change their VID/PID, these might not work correctly.

### Wireless adapters
- Xbox 360 PC adapter (Microsoft or clones)
- 8Bitdo v1 and v2 Bluetooth adapters (set to XInput mode)
- Most wireless adapters that present themselves as Switch/XInput/PlayStation controllers should work

### Wireless Bluetooth controllers (Pico W & ESP32)
**Note:** Bluetooth functionality is in early testing, some may have quirks.
- Xbox Series, One, and Elite 2
- Dualshock 3
- Dualshock 4
- Dualsense
- Switch Pro
- Steam
- Stadia
- And more

Please visit [**this page**](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/) for a more comprehensive list of supported controllers and Bluetooth pairing instructions.

## Features new to v1.1.1 (by Fred)

### Xbox 360 Rock Band Guitar Driver:
- **Xbox 360 Guitar Emulation**: The PS3 Guitar can now be used with the Xbox 360 in Rock Band games. The device is recognized as a **Rock Band Guitar** (not Guitar Hero), so it only works with games that support Rock Band controllers.
- **Limitation**: Guitar Hero 2 and 3 are **not supported** because they require Guitar Hero-specific controllers. However, Rock Band 2, 3, and later games work correctly.
- **Known Issue**: The **tilt sensor** feature (raising the guitar) could not be emulated, so the tilt functionality is not available.

### PS2→PS3 Adapter Support (Neo Brand):
- **New Controller Support**: Added support for the **Neo brand PS2 to PS3 USB adapter**. The adapter allows you to use PS2 controllers on the PS3 and is now recognized by OGX-Mini.
- **Vibration Disabled**: To prevent controller disconnections due to power draw issues, vibration feedback has been disabled for this adapter.
- **Other Adapters**: Other PS2 USB adapters I tested were not recognized. Only the Neo brand adapter is confirmed to work.

### Bug Fixes:
- Fixed a regression in the PS3 controller driver that caused input delays and "stuck" inputs.

---

## Features new to v1.1.0 (by Fred)

### PS3 Driver Fixes:
- Fixed a bug that prevented playing Guitar Hero 3 and Gran Turismo 6.
- Fixed an issue where pressing the Home/Guide button on the controller would trigger multiple phantom inputs. The button now functions correctly.

### New Drivers:

- **PlayStation 4 (DS4) Driver**: Added a new driver that emulates the authentic Sony DualShock 4 (VID: 054C, PID: 09CC). This driver is natively recognized by the PlayStation 3, providing proper analog triggers (L2/R2) and Select button functionality. Activation: Start + Dpad Left + A.

- **360 Guitar Driver (Native)**: Added a native Xbox 360 guitar driver (VID: 045E, PID: 028E) selectable via Start + Dpad Up + A. This allows the device to be recognized specifically as a guitar by the Xbox 360, enabling support for games that require authentic guitar controllers.
## Build Information:

This version was compiled and tested only for the Pi Pico, as it is the only board I currently have available for testing.

## Known Issues

The issue with phantom inputs has not yet been resolved. It occurs in some games such as God of War 3 (D-pad left and right analog stick up) and GTA 5 (D-pad up).

## Features new to v1.0.0
- Bluetooth functionality for the Pico W, Pico 2 W, and Pico+ESP32.
- Web application (connectable via USB or Bluetooth) for configuring deadzones and buttons mappings, supports up to 8 saved profiles.
- Pi Pico 2 and Pico 2 W (RP2350) support.
- Reduced latency by about 3-4 ms, graphs showing comparisons are coming.
- 4 channel functionality, connect 4 Picos and use one Xbox 360 wireless adapter to control all 4.
- Delayed USB mount until a controller is plugged in, useful for internal installation (non-Bluetooth boards only).
- Generic HID controller support.
- Dualshock 3 emulation (minus gyros), rumble now works.
- Steel Battalion controller emulation with a wireless Xbox 360 chatpad.
- Xbox DVD dongle emulation. You must provide or dump your own dongle firmware, see the Tools directory.
- Analog button support on OG Xbox and PS3.
- RGB LED support for RP2040-Zero and Adafruit Feather boards.

## Planned additions
- More accurate report parser for unknown HID controllers
- Hardware design for internal OG Xbox install
- Hardware design for 4 channel RP2040-Zero adapter
- Wired Xbox 360 chatpad support
- Wired Xbox One chatpad support
- Switch (as input) rumble support
- OG Xbox communicator support (in some form)
- Generic bluetooth dongle support
- Button macros
- Rumble settings (intensity, enabled/disable, etc.)

## Hardware

For Pi Pico, RP2040-Zero, 4 channel, and ESP32 configurations, please see the hardware folder for diagrams.

I've designed a PCB for the RP2040-Zero so you can make a small form-factor adapter yourself. The gerber files, schematic, and BOM are in Hardware folder.

<img src="images/OGX-Mini-rpzero-int.jpg" alt="OGX-Mini Boards" width="400">


If you would like a prebuilt unit, you can purchase one, with cable and Xbox adapter included, from my Etsy store.

Adding supported controllers

If your third party controller isn't working, but the original version is listed above, send me the device's VID and PID and I'll add it so it's recognized properly.

## Build

For complete build instructions, requirements, and troubleshooting, see **[CompileHelp.md](CompileHelp.md)**.

### Quick Start

```bash
git clone --recursive https://github.com/guimaraf/OGX-Mini-Plus.git
cd OGX-Mini-Plus/Firmware/RP2040
cmake -S . -B build_pico -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO
cmake --build build_pico
```

Supported boards: `PI_PICO`, `PI_PICO2`, `PI_PICOW`, `PI_PICO2W`, `RP2040_ZERO`, `ADAFRUIT_FEATHER`

### ESP32

Please see the Hardware directory for ESP32 setup instructions.

