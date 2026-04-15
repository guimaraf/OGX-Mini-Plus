#ifndef _SDK_CONFIG_H_
#define _SDK_CONFIG_H_

#include "Board/Config.h"

//
// Emulate "menuconfig"
//
#define CONFIG_BLUEPAD32_MAX_DEVICES MAX_GAMEPADS
#define CONFIG_BLUEPAD32_MAX_ALLOWLIST 4
// CONFIG_BLUEPAD32_GAP_SECURITY not defined -> GAP Level defaults to 0 (LEVEL_0)
// This allows incoming HID connections without L2CAP authentication (e.g. Switch Pro Controller clones).
// Recommended approach by btstack maintainer: use gap_set_security_level(LEVEL_0) at the app level
// instead of patching the btstack internals. Safe for OGX-Mini-Plus as it stores no sensitive data.
// #define CONFIG_BLUEPAD32_GAP_SECURITY 1
#define CONFIG_BLUEPAD32_ENABLE_BLE_BY_DEFAULT 1
// #define CONFIG_BLUEPAD32_ENABLE_VIRTUAL_DEVICE_BY_DEFAULT 1

#define CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#define CONFIG_TARGET_PICO_W

// 2 == Info
#define CONFIG_BLUEPAD32_LOG_LEVEL 0

#endif //_SDK_CONFIG_H_