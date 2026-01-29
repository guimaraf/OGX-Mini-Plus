# OGX-Mini Plus - Dependencies

> **Project Version:** v1.1.1  
> **Last Updated:** 2026-01-29

This document lists all libraries and dependencies used in the project, their exact versions, repository links, and any modifications applied.

---

## Dependencies Table

| Library | Version | Repository | Fork/Original | Branch |
|---------|---------|------------|---------------|--------|
| **Pico SDK** | 2.1.1 | [guimaraf/pico-sdk](https://github.com/guimaraf/pico-sdk) | Fork ✅ | `patched-2.1.1` |
| **TinyUSB** | 0.17.0 | [hathach/tinyusb](https://github.com/hathach/tinyusb) | Original | `0.17.0` |
| **Bluepad32** | 4.1.0 | [guimaraf/bluepad32](https://github.com/guimaraf/bluepad32) | Fork ✅ | `main` |
| **BTStack** | v1.6.1 | [guimaraf/btstack](https://github.com/guimaraf/btstack) | Fork ✅ | `patched-v1.6.1` |
| **Pico-PIO-USB** | latest | [wiredopposite/Pico-PIO-USB](https://github.com/wiredopposite/Pico-PIO-USB) | Original | `main` |
| **libfixmath** | latest | [PetteriAimonen/libfixmath](https://github.com/PetteriAimonen/libfixmath) | Original | `main` |

---

## Patches Applied to Forks

### 1. Pico SDK - `pioasm` GCC 14.x Compatibility

**Files modified:**
- `tools/pioasm/pio_types.h`
- `tools/pioasm/output_format.h`

**Change:** Added `#include <cstdint>` for GCC 14.x compatibility.

```diff
 #ifndef _PIO_TYPES_H
 #define _PIO_TYPES_H
 
+#include <cstdint>
 #include <string>
```

---

### 2. BTStack - L2CAP Security Bypass

**File modified:** `src/l2cap.c`

**Change:** Bypass security level check for controller compatibility.

```diff
 channel->state = L2CAP_STATE_WAIT_INCOMING_SECURITY_LEVEL_UPDATE;
+#if 0
 if (channel->required_security_level <= gap_security_level(channel->con_handle)){
+#else
+if (1) {
+#endif
     l2cap_handle_security_level_incoming_sufficient(channel);
```

---

### 3. Bluepad32 - HID Parser Includes

**File modified:** `src/components/bluepad32/include/uni.h`

**Change:** Added HID parser includes for additional controller support.

```diff
+#include "parser/uni_hid_parser_ds4.h"
+#include "parser/uni_hid_parser_ds3.h"
+#include "parser/uni_hid_parser_switch.h"
+#include "parser/uni_hid_parser_stadia.h"
+#include "parser/uni_hid_parser_psmove.h"
+#include "parser/uni_hid_parser_wii.h"
```

---

## Build Tools Required

| Tool | Minimum Version | Tested Version |
|------|-----------------|----------------|
| Git | 2.x | 2.49.0 |
| Python | 3.8+ | 3.10.11 |
| CMake | 3.13+ | 4.2.1 |
| Ninja | 1.10+ | 1.13.2 |
| ARM GCC Toolchain | 12.x+ | 14.2.1 |

---

## Important Notes

1. **All patches are pre-applied in forks** - No manual patching required when cloning.

2. **TinyUSB version constraint** - Must use exactly `0.17.0`. Version `0.18.0+` has breaking API changes.

3. **Pico SDK version constraint** - Must use `2.1.1`. Version `2.2.0` includes TinyUSB 0.18.0 which is incompatible.
