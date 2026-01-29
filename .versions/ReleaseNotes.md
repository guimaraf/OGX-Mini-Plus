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

---

## 🔗 Links

- [Web App for Configuration](https://wiredopposite.github.io/OGX-Mini-WebApp/)
- [Original Project Repository](https://github.com/wiredopposite/OGX-Mini)