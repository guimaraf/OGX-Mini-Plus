#ifndef _PS4_DESCRIPTORS_H_
#define _PS4_DESCRIPTORS_H_

#include <cstdint>

namespace PS4 {
static constexpr uint8_t DPAD_MASK = 0x0F;
static constexpr uint8_t COUNTER_MASK = 0xFC;
static constexpr uint8_t AXIS_MAX = 0xFF;
static constexpr uint8_t AXIS_MIN = 0x00;
static constexpr uint8_t JOYSTICK_MID = 0x80;

namespace Buttons0 {
static constexpr uint8_t DPAD_UP = 0x00;
static constexpr uint8_t DPAD_UP_RIGHT = 0x01;
static constexpr uint8_t DPAD_RIGHT = 0x02;
static constexpr uint8_t DPAD_RIGHT_DOWN = 0x03;
static constexpr uint8_t DPAD_DOWN = 0x04;
static constexpr uint8_t DPAD_DOWN_LEFT = 0x05;
static constexpr uint8_t DPAD_LEFT = 0x06;
static constexpr uint8_t DPAD_LEFT_UP = 0x07;
static constexpr uint8_t DPAD_CENTER = 0x08;

static constexpr uint8_t SQUARE = 0x10;
static constexpr uint8_t CROSS = 0x20;
static constexpr uint8_t CIRCLE = 0x40;
static constexpr uint8_t TRIANGLE = 0x80;
}; // namespace Buttons0

namespace Buttons1 {
static constexpr uint8_t L1 = 0x01;
static constexpr uint8_t R1 = 0x02;
static constexpr uint8_t L2 = 0x04;
static constexpr uint8_t R2 = 0x08;
static constexpr uint8_t SHARE = 0x10;
static constexpr uint8_t OPTIONS = 0x20;
static constexpr uint8_t L3 = 0x40;
static constexpr uint8_t R3 = 0x80;
}; // namespace Buttons1

namespace Buttons2 {
static constexpr uint8_t PS = 0x01;
static constexpr uint8_t TP = 0x02;
}; // namespace Buttons2

#pragma pack(push, 1)
// Formato real do DS4 - 64 bytes
struct InReport {
  uint8_t report_id; // 0: Report ID (0x01)

  uint8_t joystick_lx; // 1: Left stick X
  uint8_t joystick_ly; // 2: Left stick Y
  uint8_t joystick_rx; // 3: Right stick X
  uint8_t joystick_ry; // 4: Right stick Y

  // Byte 5: D-pad (lower 4 bits) + face buttons (upper 4 bits)
  uint8_t buttons0; // 5: D-pad and face buttons

  // Byte 6: shoulder + menu buttons
  uint8_t buttons1; // 6: L1, R1, L2, R2, Share, Options, L3, R3

  // Byte 7: system buttons + counter
  uint8_t buttons2; // 7: PS, Touchpad, counter

  // Bytes 8-9: Analog triggers (CRÍTICO para jogos!)
  uint8_t trigger_l; // 8: L2 axis (0-255)
  uint8_t trigger_r; // 9: R2 axis (0-255)

  // Bytes 10-63: Sensor data, touchpad, etc. (podemos deixar zerado)
  uint8_t timestamp[2];   // 10-11
  uint8_t battery;        // 12
  int16_t gyro[3];        // 13-18
  int16_t accel[3];       // 19-24
  uint8_t reserved1[5];   // 25-29
  uint8_t battery_level;  // 30
  uint8_t reserved2[2];   // 31-32
  uint8_t touch_count;    // 33
  uint8_t touch_data[30]; // 34-63
};
static_assert(sizeof(InReport) == 64);

struct OutReport {
  uint8_t report_id;

  uint8_t set_rumble : 1;
  uint8_t set_led : 1;
  uint8_t set_led_blink : 1;
  uint8_t set_ext_write : 1;
  uint8_t set_left_volume : 1;
  uint8_t set_right_volume : 1;
  uint8_t set_mic_volume : 1;
  uint8_t set_speaker_volume : 1;
  uint8_t set_flags2;

  uint8_t reserved;

  uint8_t motor_right;
  uint8_t motor_left;

  uint8_t lightbar_red;
  uint8_t lightbar_green;
  uint8_t lightbar_blue;
  uint8_t lightbar_blink_on;
  uint8_t lightbar_blink_off;

  uint8_t ext_data[8];

  uint8_t volume_left;
  uint8_t volume_right;
  uint8_t volume_mic;
  uint8_t volume_speaker;

  uint8_t other[9];
};
static_assert(sizeof(OutReport) == 32);
#pragma pack(pop)

static const uint8_t LED_COLORS[][3] = {
    {0x00, 0x00, 0x40}, // Blue
    {0x40, 0x00, 0x00}, // Red
    {0x00, 0x40, 0x00}, // Green
    {0x20, 0x00, 0x20}, // Pink
};

// String descriptors
static const uint8_t STRING_LANGUAGE[] = {0x09, 0x04};
static const uint8_t STRING_MANUFACTURER[] = "Almeids Games";
static const uint8_t STRING_PRODUCT[] = "Virtual PS4 Controller";
static const uint8_t STRING_VERSION[] = "1.0";

static const uint8_t *STRING_DESCRIPTORS[] __attribute__((unused)) = {
    STRING_LANGUAGE, STRING_MANUFACTURER, STRING_PRODUCT, STRING_VERSION};

// Device descriptor - Generic HID Gamepad (not official Sony VID/PID)
static const uint8_t DEVICE_DESCRIPTORS[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB 2.00
    0x00, // bDeviceClass (Use class information in the Interface Descriptors)
    0x00, // bDeviceSubClass
    0x00, // bDeviceProtocol
    0x40, // bMaxPacketSize0 64
    0x1D, 0x0D, // idVendor 0x0D1D (Almeids Games)
    0x04, 0x00, // idProduct 0x0004
    0x00, 0x01, // bcdDevice 1.00
    0x01,       // iManufacturer (String Index)
    0x02,       // iProduct (String Index)
    0x00,       // iSerialNumber (String Index)
    0x01,       // bNumConfigurations 1
};

// HID Report Descriptor - DS4 format (64 bytes)
static const uint8_t REPORT_DESCRIPTORS[] = {
    0x05,
    0x01, // Usage Page (Generic Desktop Ctrls)
    0x09,
    0x05, // Usage (Game Pad)
    0xA1,
    0x01, // Collection (Application)
    0x85,
    0x01, //   Report ID (1)

    // === Joysticks - 4 axes (LX, LY, RX, RY) = 4 bytes ===
    0x09,
    0x30, //   Usage (X)
    0x09,
    0x31, //   Usage (Y)
    0x09,
    0x32, //   Usage (Z)
    0x09,
    0x35, //   Usage (Rz)
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
    0x02, //   Input (Data,Var,Abs)

    // === buttons0: D-pad HAT (4 bits) + face buttons (4 bits) = 1 byte ===
    0x09,
    0x39, //   Usage (Hat switch)
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
    0x14, //   Unit (English Rotation)
    0x75,
    0x04, //   Report Size (4)
    0x95,
    0x01, //   Report Count (1)
    0x81,
    0x42, //   Input (Data,Var,Abs,Null)

    // Face buttons (4 bits) - Square, Cross, Circle, Triangle
    0x65,
    0x00, //   Unit (None)
    0x05,
    0x09, //   Usage Page (Button)
    0x19,
    0x01, //   Usage Minimum (Button 1)
    0x29,
    0x04, //   Usage Maximum (Button 4)
    0x15,
    0x00, //   Logical Minimum (0)
    0x25,
    0x01, //   Logical Maximum (1)
    0x75,
    0x01, //   Report Size (1)
    0x95,
    0x04, //   Report Count (4)
    0x81,
    0x02, //   Input (Data,Var,Abs)

    // === buttons1: L1, R1, L2, R2, Share, Options, L3, R3 = 1 byte ===
    0x19,
    0x05, //   Usage Minimum (Button 5)
    0x29,
    0x0C, //   Usage Maximum (Button 12)
    0x95,
    0x08, //   Report Count (8)
    0x81,
    0x02, //   Input (Data,Var,Abs)

    // === buttons2: PS, Touchpad + 6 padding bits = 1 byte ===
    0x19,
    0x0D, //   Usage Minimum (Button 13)
    0x29,
    0x0E, //   Usage Maximum (Button 14)
    0x95,
    0x02, //   Report Count (2)
    0x81,
    0x02, //   Input (Data,Var,Abs)

    // Padding (6 bits para completar buttons2)
    0x75,
    0x06, //   Report Size (6)
    0x95,
    0x01, //   Report Count (1)
    0x81,
    0x03, //   Input (Const,Var,Abs)

    // === Analog Triggers L2, R2 = 2 bytes ===
    0x05,
    0x01, //   Usage Page (Generic Desktop Ctrls)
    0x09,
    0x33, //   Usage (Rx) - L2 analog
    0x09,
    0x34, //   Usage (Ry) - R2 analog
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
    0x02, //   Input (Data,Var,Abs)

    // === Remaining bytes (10-63) = 54 bytes of sensor/touchpad data ===
    0x06,
    0x00,
    0xFF, //   Usage Page (Vendor Defined 0xFF00)
    0x09,
    0x21, //   Usage (0x21) - Sensor data
    0x95,
    0x36, //   Report Count (54)
    0x81,
    0x02, //   Input (Data,Var,Abs)

    // Output report for rumble/LED (32 bytes)
    0x09,
    0x20, //   Usage (0x20)
    0x95,
    0x20, //   Report Count (32)
    0x91,
    0x02, //   Output (Data,Var,Abs)

    0xC0, // End Collection
};

static const uint8_t CONFIGURATION_DESCRIPTORS[] = {
    0x09, // bLength
    0x02, // bDescriptorType (Configuration)
    0x29,
    0x00, // wTotalLength 41
    0x01, // bNumInterfaces 1
    0x01, // bConfigurationValue
    0x00, // iConfiguration (String Index)
    0x80, // bmAttributes
    0xFA, // bMaxPower 500mA

    0x09, // bLength
    0x04, // bDescriptorType (Interface)
    0x00, // bInterfaceNumber 0
    0x00, // bAlternateSetting
    0x02, // bNumEndpoints 2
    0x03, // bInterfaceClass (HID)
    0x00, // bInterfaceSubClass
    0x00, // bInterfaceProtocol
    0x00, // iInterface (String Index)

    0x09, // bLength
    0x21, // bDescriptorType (HID)
    0x11,
    0x01, // bcdHID 1.17
    0x00, // bCountryCode
    0x01, // bNumDescriptors
    0x22, // bDescriptorType[0] (HID)
    sizeof(REPORT_DESCRIPTORS),
    0x00, // wDescriptorLength[0]

    0x07, // bLength
    0x05, // bDescriptorType (Endpoint)
    0x02, // bEndpointAddress (OUT/H2D)
    0x03, // bmAttributes (Interrupt)
    0x40,
    0x00, // wMaxPacketSize 64
    0x05, // bInterval 5ms

    0x07, // bLength
    0x05, // bDescriptorType (Endpoint)
    0x81, // bEndpointAddress (IN/D2H)
    0x03, // bmAttributes (Interrupt)
    0x40,
    0x00, // wMaxPacketSize 64
    0x05, // bInterval 5ms
};

}; // namespace PS4

#endif // _PS4_DESCRIPTORS_H_