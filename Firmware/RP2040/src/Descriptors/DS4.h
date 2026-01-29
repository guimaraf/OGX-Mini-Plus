#ifndef _DS4_DESCRIPTORS_H_
#define _DS4_DESCRIPTORS_H_

#include <cstdint>

// DualShock 4 (Wireless Controller) - VID: 054C, PID: 09CC
// Based on real DS4 v2 HID descriptors captured via USBTreeView

namespace DS4 {

// Constants
static constexpr uint8_t JOYSTICK_MID = 0x80; // 128 center
static constexpr uint8_t AXIS_MIN = 0x00;
static constexpr uint8_t AXIS_MAX = 0xFF;

// D-pad HAT switch values
namespace DPad {
static constexpr uint8_t UP = 0x00;
static constexpr uint8_t UP_RIGHT = 0x01;
static constexpr uint8_t RIGHT = 0x02;
static constexpr uint8_t DOWN_RIGHT = 0x03;
static constexpr uint8_t DOWN = 0x04;
static constexpr uint8_t DOWN_LEFT = 0x05;
static constexpr uint8_t LEFT = 0x06;
static constexpr uint8_t UP_LEFT = 0x07;
static constexpr uint8_t CENTER = 0x08;
}; // namespace DPad

// Button masks for buttons0 (lower nibble is D-pad, upper nibble is face
// buttons)
namespace Buttons0 {
static constexpr uint8_t DPAD_MASK = 0x0F;
static constexpr uint8_t SQUARE = 0x10;   // Bit 4
static constexpr uint8_t CROSS = 0x20;    // Bit 5
static constexpr uint8_t CIRCLE = 0x40;   // Bit 6
static constexpr uint8_t TRIANGLE = 0x80; // Bit 7
}; // namespace Buttons0

// Button masks for buttons1
namespace Buttons1 {
static constexpr uint8_t L1 = 0x01;      // Bit 0
static constexpr uint8_t R1 = 0x02;      // Bit 1
static constexpr uint8_t L2 = 0x04;      // Bit 2 (digital trigger)
static constexpr uint8_t R2 = 0x08;      // Bit 3 (digital trigger)
static constexpr uint8_t SHARE = 0x10;   // Bit 4 (SELECT on PS3)
static constexpr uint8_t OPTIONS = 0x20; // Bit 5 (START on PS3)
static constexpr uint8_t L3 = 0x40;      // Bit 6
static constexpr uint8_t R3 = 0x80;      // Bit 7
}; // namespace Buttons1

// Button masks for buttons2
namespace Buttons2 {
static constexpr uint8_t PS = 0x01; // Bit 0 (counter uses bits 2-7)
static constexpr uint8_t TP = 0x02; // Bit 1 (Touchpad click)
}; // namespace Buttons2

#pragma pack(push, 1)

// Input Report - 64 bytes (Report ID 0x01)
// Based on real DS4 format from captured descriptors
struct InReport {
  uint8_t report_id; // 0: Always 0x01

  // Joysticks (bytes 1-4)
  uint8_t joystick_lx; // 1: Left stick X (0=left, 128=center, 255=right)
  uint8_t joystick_ly; // 2: Left stick Y (0=up, 128=center, 255=down)
  uint8_t joystick_rx; // 3: Right stick X
  uint8_t joystick_ry; // 4: Right stick Y

  // Buttons (bytes 5-7)
  uint8_t buttons0; // 5: D-pad (lower 4 bits) + Square/Cross/Circle/Triangle
                    // (upper 4 bits)
  uint8_t buttons1; // 6: L1/R1/L2/R2/Share/Options/L3/R3
  uint8_t buttons2; // 7: PS button (bit 0) + TP (bit 1) + counter (bits 2-7)

  // Analog triggers (bytes 8-9) - CRITICAL for PS3 compatibility!
  uint8_t trigger_l; // 8: L2 analog (0-255)
  uint8_t trigger_r; // 9: R2 analog (0-255)

  // Timestamp (bytes 10-11)
  uint16_t timestamp; // 10-11: Timestamp counter

  // Battery (byte 12)
  uint8_t battery; // 12: Battery level

  // Gyro/Accel (bytes 13-24)
  int16_t gyro_x;  // 13-14
  int16_t gyro_y;  // 15-16
  int16_t gyro_z;  // 17-18
  int16_t accel_x; // 19-20
  int16_t accel_y; // 21-22
  int16_t accel_z; // 23-24

  // Reserved (bytes 25-29)
  uint8_t reserved1[5]; // 25-29

  // Battery/Status (byte 30)
  uint8_t battery_level; // 30: 0x00-0x0B full, cable: 0x10-0x1B, 0xEE charging

  // Reserved (bytes 31-32)
  uint8_t reserved2[2]; // 31-32

  // Touchpad (bytes 33-63)
  uint8_t touch_count;    // 33: Number of touch points
  uint8_t touch_data[30]; // 34-63: Touch data
};
static_assert(sizeof(InReport) == 64, "DS4::InReport must be 64 bytes");

// Output Report - 32 bytes (Report ID 0x05)
struct OutReport {
  uint8_t report_id; // 0: Report ID (0x05)

  uint8_t flags;     // 1: Flags
  uint8_t reserved1; // 2: Reserved

  uint8_t motor_right; // 3: Right motor (weak, high freq)
  uint8_t motor_left;  // 4: Left motor (strong, low freq)

  uint8_t lightbar_red;       // 5: Lightbar red
  uint8_t lightbar_green;     // 6: Lightbar green
  uint8_t lightbar_blue;      // 7: Lightbar blue
  uint8_t lightbar_blink_on;  // 8: Blink on duration
  uint8_t lightbar_blink_off; // 9: Blink off duration

  uint8_t reserved2[22]; // 10-31: Reserved
};
static_assert(sizeof(OutReport) == 32, "DS4::OutReport must be 32 bytes");

#pragma pack(pop)

// String descriptors
static const uint8_t STRING_LANGUAGE[] = {0x09, 0x04};
static const uint8_t STRING_MANUFACTURER[] = "Sony Interactive Entertainment";
static const uint8_t STRING_PRODUCT[] = "Wireless Controller";
static const uint8_t STRING_VERSION[] = "1.0";

static const uint8_t *STRING_DESCRIPTORS[] __attribute__((unused)) = {
    STRING_LANGUAGE, STRING_MANUFACTURER, STRING_PRODUCT, STRING_VERSION};

// Device Descriptor - Sony DualShock 4 V2
static const uint8_t DEVICE_DESCRIPTORS[] = {
    0x12,       // bLength (18 bytes)
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB 2.00
    0x00,       // bDeviceClass (defined by interface)
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    0x40,       // bMaxPacketSize0 (64 bytes)
    0x4C, 0x05, // idVendor 0x054C (Sony)
    0xCC, 0x09, // idProduct 0x09CC (Wireless Controller)
    0x00, 0x01, // bcdDevice 1.00
    0x01,       // iManufacturer (String Index 1)
    0x02,       // iProduct (String Index 2)
    0x00,       // iSerialNumber (No String)
    0x01,       // bNumConfigurations (1)
};

// HID Report Descriptor - Exact copy from real DS4
// 507 bytes as captured from USBTreeView
static const uint8_t REPORT_DESCRIPTORS[] = {
    0x05,
    0x01, // Usage Page (Generic Desktop Controls)
    0x09,
    0x05, // Usage (Gamepad)
    0xA1,
    0x01, // Collection (Application)
    0x85,
    0x01, //   Report ID (0x01)

    // Joysticks (4 bytes: LX, LY, RX, RY)
    0x09,
    0x30, //   Usage (Direction-X)
    0x09,
    0x31, //   Usage (Direction-Y)
    0x09,
    0x32, //   Usage (Direction-Z)
    0x09,
    0x35, //   Usage (Rotate-Z)
    0x15,
    0x00, //   Logical Minimum (0)
    0x26,
    0xFF,
    0x00, //   Logical Maximum (255)
    0x75,
    0x08, //   Report Size (8)
    0x95,
    0x04, //   Report Count (4)
    0x81,
    0x02, //   Input (Var)

    // D-pad HAT switch (4 bits)
    0x09,
    0x39, //   Usage (Hat Switch)
    0x15,
    0x00, //   Logical Minimum (0)
    0x25,
    0x07, //   Logical Maximum (7)
    0x35,
    0x00, //   Physical Minimum (0)
    0x46,
    0x3B,
    0x01, //   Physical Maximum (315)
    0x65,
    0x14, //   Unit (Degrees)
    0x75,
    0x04, //   Report Size (4)
    0x95,
    0x01, //   Report Count (1)
    0x81,
    0x42, //   Input (Var, NullState)

    // Buttons 1-14 (14 bits)
    0x65,
    0x00, //   Unit (None)
    0x05,
    0x09, //   Usage Page (Buttons)
    0x19,
    0x01, //   Usage Minimum (1)
    0x29,
    0x0E, //   Usage Maximum (14)
    0x15,
    0x00, //   Logical Minimum (0)
    0x25,
    0x01, //   Logical Maximum (1)
    0x75,
    0x01, //   Report Size (1)
    0x95,
    0x0E, //   Report Count (14)
    0x81,
    0x02, //   Input (Var)

    // Padding/Counter (6 bits)
    0x06,
    0x00,
    0xFF, //   Usage Page (Vendor Defined)
    0x09,
    0x20, //   Usage (unknown)
    0x75,
    0x06, //   Report Size (6)
    0x95,
    0x01, //   Report Count (1)
    0x15,
    0x00, //   Logical Minimum (0)
    0x25,
    0x7F, //   Logical Maximum (127)
    0x81,
    0x02, //   Input (Var)

    // Analog Triggers L2/R2 (2 bytes) - CRITICAL!
    0x05,
    0x01, //   Usage Page (Generic Desktop Controls)
    0x09,
    0x33, //   Usage (Rotate-X) - L2
    0x09,
    0x34, //   Usage (Rotate-Y) - R2
    0x15,
    0x00, //   Logical Minimum (0)
    0x26,
    0xFF,
    0x00, //   Logical Maximum (255)
    0x75,
    0x08, //   Report Size (8)
    0x95,
    0x02, //   Report Count (2)
    0x81,
    0x02, //   Input (Var)

    // Sensor/Touchpad data (54 bytes)
    0x06,
    0x00,
    0xFF, //   Usage Page (Vendor Defined)
    0x09,
    0x21, //   Usage (unknown)
    0x95,
    0x36, //   Report Count (54)
    0x81,
    0x02, //   Input (Var)

    // Output Report (Report ID 0x05, 31 bytes)
    0x85,
    0x05, //   Report ID (0x05)
    0x09,
    0x22, //   Usage (unknown)
    0x95,
    0x1F, //   Report Count (31)
    0x91,
    0x02, //   Output (Var)

    // Feature Reports (minimal set for compatibility)
    0x85,
    0x04, //   Report ID (0x04)
    0x09,
    0x23, //   Usage (unknown)
    0x95,
    0x24, //   Report Count (36)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x02, //   Report ID (0x02)
    0x09,
    0x24, //   Usage (unknown)
    0x95,
    0x24, //   Report Count (36)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x08, //   Report ID (0x08)
    0x09,
    0x25, //   Usage (unknown)
    0x95,
    0x03, //   Report Count (3)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x10, //   Report ID (0x10)
    0x09,
    0x26, //   Usage (unknown)
    0x95,
    0x04, //   Report Count (4)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x11, //   Report ID (0x11)
    0x09,
    0x27, //   Usage (unknown)
    0x95,
    0x02, //   Report Count (2)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x12, //   Report ID (0x12)
    0x06,
    0x02,
    0xFF, //   Usage Page (Vendor Defined)
    0x09,
    0x21, //   Usage (unknown)
    0x95,
    0x0F, //   Report Count (15)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x13, //   Report ID (0x13)
    0x09,
    0x22, //   Usage (unknown)
    0x95,
    0x16, //   Report Count (22)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x14, //   Report ID (0x14)
    0x06,
    0x05,
    0xFF, //   Usage Page (Vendor Defined)
    0x09,
    0x20, //   Usage (unknown)
    0x95,
    0x10, //   Report Count (16)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x15, //   Report ID (0x15)
    0x09,
    0x21, //   Usage (unknown)
    0x95,
    0x2C, //   Report Count (44)
    0xB1,
    0x02, //   Feature (Var)

    0x06,
    0x80,
    0xFF, //   Usage Page (Vendor Defined)

    0x85,
    0x80, //   Report ID (0x80)
    0x09,
    0x20, //   Usage (unknown)
    0x95,
    0x06, //   Report Count (6)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x81, //   Report ID (0x81)
    0x09,
    0x21, //   Usage (unknown)
    0x95,
    0x06, //   Report Count (6)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x82, //   Report ID (0x82)
    0x09,
    0x22, //   Usage (unknown)
    0x95,
    0x05, //   Report Count (5)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x83, //   Report ID (0x83)
    0x09,
    0x23, //   Usage (unknown)
    0x95,
    0x01, //   Report Count (1)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x84, //   Report ID (0x84)
    0x09,
    0x24, //   Usage (unknown)
    0x95,
    0x04, //   Report Count (4)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x85, //   Report ID (0x85)
    0x09,
    0x25, //   Usage (unknown)
    0x95,
    0x06, //   Report Count (6)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x86, //   Report ID (0x86)
    0x09,
    0x26, //   Usage (unknown)
    0x95,
    0x06, //   Report Count (6)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x87, //   Report ID (0x87)
    0x09,
    0x27, //   Usage (unknown)
    0x95,
    0x23, //   Report Count (35)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x88, //   Report ID (0x88)
    0x09,
    0x28, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x89, //   Report ID (0x89)
    0x09,
    0x29, //   Usage (unknown)
    0x95,
    0x02, //   Report Count (2)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x90, //   Report ID (0x90)
    0x09,
    0x30, //   Usage (unknown)
    0x95,
    0x05, //   Report Count (5)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x91, //   Report ID (0x91)
    0x09,
    0x31, //   Usage (unknown)
    0x95,
    0x03, //   Report Count (3)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x92, //   Report ID (0x92)
    0x09,
    0x32, //   Usage (unknown)
    0x95,
    0x03, //   Report Count (3)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x93, //   Report ID (0x93)
    0x09,
    0x33, //   Usage (unknown)
    0x95,
    0x0C, //   Report Count (12)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0x94, //   Report ID (0x94)
    0x09,
    0x34, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xA0, //   Report ID (0xA0)
    0x09,
    0x40, //   Usage (unknown)
    0x95,
    0x06, //   Report Count (6)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xA1, //   Report ID (0xA1)
    0x09,
    0x41, //   Usage (unknown)
    0x95,
    0x01, //   Report Count (1)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xA2, //   Report ID (0xA2)
    0x09,
    0x42, //   Usage (unknown)
    0x95,
    0x01, //   Report Count (1)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xA3, //   Report ID (0xA3)
    0x09,
    0x43, //   Usage (unknown)
    0x95,
    0x30, //   Report Count (48)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xA4, //   Report ID (0xA4)
    0x09,
    0x44, //   Usage (unknown)
    0x95,
    0x0D, //   Report Count (13)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xF0, //   Report ID (0xF0)
    0x09,
    0x47, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xF1, //   Report ID (0xF1)
    0x09,
    0x48, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xF2, //   Report ID (0xF2)
    0x09,
    0x49, //   Usage (unknown)
    0x95,
    0x0F, //   Report Count (15)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xA7, //   Report ID (0xA7)
    0x09,
    0x4A, //   Usage (unknown)
    0x95,
    0x01, //   Report Count (1)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xA8, //   Report ID (0xA8)
    0x09,
    0x4B, //   Usage (unknown)
    0x95,
    0x01, //   Report Count (1)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xA9, //   Report ID (0xA9)
    0x09,
    0x4C, //   Usage (unknown)
    0x95,
    0x08, //   Report Count (8)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xAA, //   Report ID (0xAA)
    0x09,
    0x4E, //   Usage (unknown)
    0x95,
    0x01, //   Report Count (1)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xAB, //   Report ID (0xAB)
    0x09,
    0x4F, //   Usage (unknown)
    0x95,
    0x39, //   Report Count (57)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xAC, //   Report ID (0xAC)
    0x09,
    0x50, //   Usage (unknown)
    0x95,
    0x39, //   Report Count (57)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xAD, //   Report ID (0xAD)
    0x09,
    0x51, //   Usage (unknown)
    0x95,
    0x0B, //   Report Count (11)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xAE, //   Report ID (0xAE)
    0x09,
    0x52, //   Usage (unknown)
    0x95,
    0x01, //   Report Count (1)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xAF, //   Report ID (0xAF)
    0x09,
    0x53, //   Usage (unknown)
    0x95,
    0x02, //   Report Count (2)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xB0, //   Report ID (0xB0)
    0x09,
    0x54, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xE0, //   Report ID (0xE0)
    0x09,
    0x57, //   Usage (unknown)
    0x95,
    0x02, //   Report Count (2)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xB3, //   Report ID (0xB3)
    0x09,
    0x55, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xB4, //   Report ID (0xB4)
    0x09,
    0x55, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xB5, //   Report ID (0xB5)
    0x09,
    0x56, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xD0, //   Report ID (0xD0)
    0x09,
    0x58, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0x85,
    0xD4, //   Report ID (0xD4)
    0x09,
    0x59, //   Usage (unknown)
    0x95,
    0x3F, //   Report Count (63)
    0xB1,
    0x02, //   Feature (Var)

    0xC0, // End Collection
};

// Configuration Descriptor - HID only (simplified, no audio)
// Total: 9 (config) + 9 (interface) + 9 (HID) + 7 (EP IN) + 7 (EP OUT) = 41
// bytes
static const uint8_t CONFIGURATION_DESCRIPTORS[] = {
    // Configuration Descriptor
    0x09, // bLength
    0x02, // bDescriptorType (Configuration)
    0x29,
    0x00, // wTotalLength (41 bytes)
    0x01, // bNumInterfaces (1)
    0x01, // bConfigurationValue
    0x00, // iConfiguration (No String)
    0xC0, // bmAttributes (Self-powered)
    0xFA, // bMaxPower (500mA)

    // Interface Descriptor (HID)
    0x09, // bLength
    0x04, // bDescriptorType (Interface)
    0x00, // bInterfaceNumber (0)
    0x00, // bAlternateSetting
    0x02, // bNumEndpoints (2)
    0x03, // bInterfaceClass (HID)
    0x00, // bInterfaceSubClass (None)
    0x00, // bInterfaceProtocol (None)
    0x00, // iInterface (No String)

    // HID Descriptor
    0x09, // bLength
    0x21, // bDescriptorType (HID)
    0x11,
    0x01, // bcdHID 1.17
    0x00, // bCountryCode
    0x01, // bNumDescriptors
    0x22, // bDescriptorType[0] (Report)
    sizeof(REPORT_DESCRIPTORS) & 0xFF,
    (sizeof(REPORT_DESCRIPTORS) >> 8) & 0xFF, // wDescriptorLength

    // Endpoint Descriptor (IN)
    0x07, // bLength
    0x05, // bDescriptorType (Endpoint)
    0x81, // bEndpointAddress (IN, Endpoint 1)
    0x03, // bmAttributes (Interrupt)
    0x40,
    0x00, // wMaxPacketSize (64 bytes)
    0x05, // bInterval (5ms)

    // Endpoint Descriptor (OUT)
    0x07, // bLength
    0x05, // bDescriptorType (Endpoint)
    0x02, // bEndpointAddress (OUT, Endpoint 2)
    0x03, // bmAttributes (Interrupt)
    0x40,
    0x00, // wMaxPacketSize (64 bytes)
    0x05, // bInterval (5ms)
};

// LED colors for player indicators
static const uint8_t LED_COLORS[][3] = {
    {0x00, 0x00, 0x40}, // Blue - Player 1
    {0x40, 0x00, 0x00}, // Red - Player 2
    {0x00, 0x40, 0x00}, // Green - Player 3
    {0x20, 0x00, 0x20}, // Pink - Player 4
};

}; // namespace DS4

#endif // _DS4_DESCRIPTORS_H_
