# Controllers and Compatibility

## Supported Platforms

- Original Xbox
- Nintendo Wii with FakeMote
- PlayStation 3
- Nintendo Switch (docked)
- XInput
- PlayStation Classic
- DInput

For Xbox 360 console use with XInput, see [UsbdSecPatch](https://github.com/InvoxiPlayGames/UsbdSecPatch) or the equivalent J-Runner patch workflow.

## Changing Platforms

By default, the adapter boots as an Original Xbox controller. To change output mode, hold the selected button combination for about 3 seconds.

`Start = Plus (Switch) = Options (DS4 / DualSense)`

- XInput: `Start + Dpad Up`
- 360 Guitar: `Start + Dpad Up + A`
- Original Xbox: `Start + Dpad Right`
- Original Xbox Steel Battalion: `Start + Dpad Right + Right Bumper`
- Original Xbox DVD Remote: `Start + Dpad Right + Left Bumper`
- Nintendo Switch: `Start + Dpad Down`
- PlayStation 3: `Start + Dpad Left`
- PlayStation 4 (DS4): `Start + Dpad Left + A`
- PlayStation Classic: `Start + A`
- Web Application Mode: `Start + Left Bumper + Right Bumper`

After saving a new mode, the RP2040 resets automatically.

## Supported Devices

### Wired Controllers

- Original Xbox Duke and Controller S
- Xbox 360, One, Series, and Elite controllers
- DualShock 3
- DualShock 4
- DualSense
- Nintendo Switch Pro
- Nintendo Switch wired controllers
- Nintendo 64 generic USB controllers
- PlayStation Classic controllers
- Generic DInput devices
- Generic HID devices

Note:
Generic HID devices may require mapping adjustments in the Web App.

### Wireless Adapters

- Xbox 360 PC wireless adapter, original and clones
- 8BitDo v1 and v2 Bluetooth adapters in XInput mode
- Most adapters that identify as Switch, XInput, or PlayStation devices

### Wireless Bluetooth Controllers

Available on Pico W, Pico 2 W, and supported ESP32 bridge setups.

- Xbox Series, One, and Elite 2
- DualShock 3
- DualShock 4
- DualSense
- Nintendo Switch Pro
- Steam Controller
- Stadia Controller
- Additional Bluepad32-supported devices

For a broader Bluetooth compatibility reference, see the [Bluepad32 supported gamepads page](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/).

## Web App

Use the Web App to adjust mappings, deadzones, and saved profiles:

- [OGX-Mini Web App](https://wiredopposite.github.io/OGX-Mini-WebApp/)

To connect over USB:

1. Plug a controller into the adapter.
2. Connect the adapter to the PC.
3. Hold `Start + Left Bumper + Right Bumper` to enter Web App mode.
4. Click `Connect via USB` in the Web App and select the device.

Bluetooth pairing with the Web App does not require entering that mode first.

## Notes on Third-Party Controllers

- Some third-party controllers can present different VID/PID combinations depending on firmware or mode.
- Some Switch Pro compatible devices need fallback handling during initialization.
- Some licensed or multi-system controllers support only a subset of original-controller features.
- USB rumble support for Switch Pro compatible devices now depends on a dedicated initialization step in the USB host driver.

## Adding Support for New Controllers

If a supported controller family still has a device that does not work correctly, the most useful data to capture is:

- VID/PID
- `lsusb -v` output
- HID report descriptor
- Input report samples
- Initialization or `usbmon` traces when available

This information is usually enough to add device-specific compatibility handling without regressing existing controllers.
