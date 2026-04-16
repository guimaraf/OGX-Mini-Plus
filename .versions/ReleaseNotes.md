# Release Notes

## Current Version

- `v1.1.4`

## v1.1.4

### Switch Pro USB Rumble

- Added USB rumble support for Switch Pro compatible controllers
- Refined the USB host initialization flow so compatible controllers can keep working without losing vibration support
- Split rumble handling into dedicated driver files to make maintenance and debugging easier

### Fork Maintenance

- Updated project versioning to `v1.1.4`
- Kept the build output version in sync with the repository release version

---

## v1.1.3

### Switch Pro Clone Support

- Fixed generic and third-party Switch Pro compatible controllers failing to mount
- Resolved cases where some controllers entered charging mode instead of becoming usable
- Added compatibility handling for USB and Bluetooth paths to support licensed and multi-system variants

---

## v1.1.2

### Switch Pro Bluetooth Triggers

- Fixed `ZL` and `ZR` not working correctly over Bluetooth
- Restored trigger compatibility on Switch Pro compatible controllers, especially on Pico 2 W builds

---

## v1.1.1

### Xbox 360 Rock Band Guitar

- Added Xbox 360 Rock Band guitar output support for compatible PS3 guitars
- The guitar is identified as a Rock Band guitar, not a Guitar Hero guitar

### PS2 to USB Adapter Support

- Added support for the Neo brand PS2 to PS3 USB adapter
- Kept vibration disabled on that adapter to avoid disconnects caused by power draw

### PS3 Driver Fixes

- Fixed input delay issues
- Fixed stuck input regressions

---

## v1.1.0

### New Drivers and Compatibility

- Added DualShock 4 output support
- Added native Xbox 360 guitar output mode
- Fixed issues affecting Guitar Hero 3 and Gran Turismo 6
- Improved Home / Guide button handling in the PS3 path

### Build and Platform Notes

- This version was compiled and tested primarily on Pi Pico hardware
- Some phantom input issues were still known at the time of release

---

## v1.0.0

- Added Bluetooth support for Pico W, Pico 2 W, and Pico plus ESP32 configurations
- Added Web App support over USB and Bluetooth
- Added Pi Pico 2 and Pico 2 W support
- Reduced latency by roughly 3 to 4 ms
- Added 4-channel support
- Added delayed USB mount for internal installs on non-Bluetooth boards
- Added generic HID support
- Added DualShock 3 emulation with rumble
- Added Steel Battalion support with Xbox 360 chatpad integration
- Added Xbox DVD dongle emulation support
- Added analog button support on Original Xbox and PS3
- Added RGB LED support for RP2040-Zero and Adafruit Feather boards

---

## Links

- [Original OGX-Mini project](https://github.com/wiredopposite/OGX-Mini)
- [Project README](../README.md)
- [Controllers and compatibility](../Controllers.md)
- [Compilation guide](../CompileHelp.md)
