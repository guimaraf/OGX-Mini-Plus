#ifndef _DEVICE_DRIVER_TYPES_H_
#define _DEVICE_DRIVER_TYPES_H_

#include <cstdint>

enum class DeviceDriverType : uint8_t {
  NONE = 0,
  XBOXOG,
  XBOXOG_SB,
  XBOXOG_XR,
  XINPUT,
  XINPUT_GUITAR_360,
  PS3,
  DS4, // Authentic DualShock 4 (VID: 054C, PID: 09CC)
  DINPUT,
  PSCLASSIC,
  SWITCH,
  WEBAPP = 100,
  UART_BRIDGE
};

#endif // _DEVICE_DRIVER_TYPES_H_