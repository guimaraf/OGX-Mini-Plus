#ifndef _XINPUT_GUITAR_360_DESCRIPTORS_H_
#define _XINPUT_GUITAR_360_DESCRIPTORS_H_

#include <cstdint>
#include <cstring>

namespace XInputGuitar360 {
static constexpr size_t ENDPOINT_IN_SIZE = 20;
static constexpr size_t ENDPOINT_OUT_SIZE = 32;

namespace Buttons0 {
static constexpr uint8_t DPAD_UP = (1U << 0);
static constexpr uint8_t DPAD_DOWN = (1U << 1);
static constexpr uint8_t DPAD_LEFT = (1U << 2);
static constexpr uint8_t DPAD_RIGHT = (1U << 3);
static constexpr uint8_t START = (1U << 4);
static constexpr uint8_t BACK = (1U << 5);
static constexpr uint8_t L3 = (1U << 6);
static constexpr uint8_t R3 = (1U << 7);
}; // namespace Buttons0

namespace Buttons1 {
static constexpr uint8_t LB = (1U << 0);
static constexpr uint8_t RB = (1U << 1);
static constexpr uint8_t HOME = (1U << 2);
static constexpr uint8_t A = (1U << 4);
static constexpr uint8_t B = (1U << 5);
static constexpr uint8_t X = (1U << 6);
static constexpr uint8_t Y = (1U << 7);
}; // namespace Buttons1

#pragma pack(push, 1)
struct InReport {
  uint8_t report_id;
  uint8_t report_size;
  uint8_t buttons[2];
  uint8_t trigger_l;
  uint8_t trigger_r;
  int16_t joystick_lx;
  int16_t joystick_ly;
  int16_t joystick_rx;
  int16_t joystick_ry;
  uint8_t reserved[6];

  InReport() {
    std::memset(this, 0, sizeof(InReport));
    report_size = sizeof(InReport);
  }
};
static_assert(sizeof(InReport) == 20, "XInputGuitar::InReport is misaligned");

struct OutReport {
  uint8_t report_id;
  uint8_t report_size;
  uint8_t led;
  uint8_t rumble_l;
  uint8_t rumble_r;
  uint8_t reserved[3];

  OutReport() { std::memset(this, 0, sizeof(OutReport)); }
};
static_assert(sizeof(OutReport) == 8, "XInputGuitar::OutReport is misaligned");
#pragma pack(pop)

static const uint8_t STRING_LANGUAGE[] = {0x09, 0x04};
static const uint8_t STRING_MANUFACTURER[] = "Harmonix Music Systems";
static const uint8_t STRING_PRODUCT[] = "Harmonix Guitar for Xbox 360";
static const uint8_t STRING_VERSION[] = "1.0";

static const uint8_t *DESC_STRING[] __attribute__((unused)) = {
    STRING_LANGUAGE, STRING_MANUFACTURER, STRING_PRODUCT, STRING_VERSION};

// Device descriptor – Harmonix VID/PID (Rock Band Guitar)
// Fixed: bMaxPacketSize0 must be 64 for RP2040 to enumerate correctly!
static const uint8_t DESC_DEVICE[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB 2.00
    0xFF,       // bDeviceClass (Vendor Specific)
    0xFF,       // bDeviceSubClass
    0xFF,       // bDeviceProtocol
    0x40,       // bMaxPacketSize0 64 (CRITICAL: 8 causes Code 43 on RP2040)
    0xAD, 0x1B, // idVendor 0x1BAD (Harmonix Music Systems)
    0x03, 0x00, // idProduct 0x0003 (Harmonix Guitar)
    0x01, 0x01, // bcdDevice 1.01
    0x01,       // iManufacturer (String Index)
    0x02,       // iProduct (String Index)
    0x03,       // iSerialNumber (String Index)
    0x01,       // bNumConfigurations 1
};

// Configuration descriptor – Xbox 360 Guitar (XInput Structure with Guitar
// Subtype)
static const uint8_t DESC_CONFIGURATION[] = {
    0x09, // bLength
    0x02, // bDescriptorType (Configuration)
    0x30,
    0x00, // wTotalLength 48
    0x01, // bNumInterfaces 1
    0x01, // bConfigurationValue
    0x00, // iConfiguration (String Index)
    0x80, // bmAttributes (Bus Powered)
    0xFA, // bMaxPower 500mA

    0x09, // bLength
    0x04, // bDescriptorType (Interface)
    0x00, // bInterfaceNumber 0
    0x00, // bAlternateSetting
    0x02, // bNumEndpoints 2
    0xFF, // bInterfaceClass (Vendor Specific)
    0x5D, // bInterfaceSubClass
    0x01, // bInterfaceProtocol
    0x00, // iInterface (String Index)

    // XInput Security/Magic Descriptor (16 bytes)
    // This exact structure is required for XInput driver enumeration
    0x10, // bLength
    0x21, // bDescriptorType (HID)
    0x00,
    0x01, // bcdHID 1.00
    0x06, // bCountryCode / XInput Subtype (0x06 = GUITAR)
    0x24, // bNumDescriptors
    0x81, // bDescriptorType[0]
    0x14,
    0x03, // wDescriptorLength[0] 788
    0x00, // bDescriptorType[1]
    0x03,
    0x13, // wDescriptorLength[1] 4867
    0x01, // bDescriptorType[2]
    0x00,
    0x03, // wDescriptorLength[2] 768
    0x00, // bDescriptorType[3]

    0x07, // bLength
    0x05, // bDescriptorType (Endpoint)
    0x81, // bEndpointAddress (IN/D2H)
    0x03, // bmAttributes (Interrupt)
    0x20,
    0x00, // wMaxPacketSize 32
    0x01, // bInterval 1 (Standard XInput)

    0x07, // bLength
    0x05, // bDescriptorType (Endpoint)
    0x01, // bEndpointAddress (OUT/H2D) - EP1 Standard
    0x03, // bmAttributes (Interrupt)
    0x20,
    0x00, // wMaxPacketSize 32
    0x08, // bInterval 8 (Standard XInput)
};

} // namespace XInputGuitar360

#endif // _XINPUT_GUITAR_360_DESCRIPTORS_H_
