#include <algorithm>
#include <cstring>

#include "USBDevice/DeviceDriver/DS4/DS4.h"

void DS4Device::initialize() {
  class_driver_ = {.name = TUD_DRV_NAME("DS4"),
                   .init = hidd_init,
                   .deinit = hidd_deinit,
                   .reset = hidd_reset,
                   .open = hidd_open,
                   .control_xfer_cb = hidd_control_xfer_cb,
                   .xfer_cb = hidd_xfer_cb,
                   .sof = NULL};
}

void DS4Device::process(const uint8_t idx, Gamepad &gamepad) {
  if (gamepad.new_pad_in()) {
    Gamepad::PadIn gp_in = gamepad.get_pad_in();

    // Initialize report with zeros
    std::memset(&report_in_, 0, sizeof(DS4::InReport));
    report_in_.report_id = 0x01;

    // === Joysticks ===
    // DS4 uses 0x80 (128) as center, range 0-255
    constexpr int16_t DEADZONE = 1640; // ~5% of 32768
    constexpr uint8_t DS4_CENTER = 0x80;

    auto joystick_to_ds4 = [](int16_t value, int16_t deadzone) -> uint8_t {
      if (value > -deadzone && value < deadzone) {
        return DS4_CENTER;
      }
      // Convert int16 (-32768 to +32767) to uint8 (0 to 255)
      int32_t scaled = ((static_cast<int32_t>(value) + 32768) * 255) / 65535;
      return static_cast<uint8_t>(scaled);
    };

    report_in_.joystick_lx = joystick_to_ds4(gp_in.joystick_lx, DEADZONE);
    report_in_.joystick_ly = joystick_to_ds4(gp_in.joystick_ly, DEADZONE);
    report_in_.joystick_rx = joystick_to_ds4(gp_in.joystick_rx, DEADZONE);
    report_in_.joystick_ry = joystick_to_ds4(gp_in.joystick_ry, DEADZONE);

    // === D-pad (HAT switch) ===
    // Lower 4 bits of buttons0
    switch (gp_in.dpad) {
    case Gamepad::DPAD_UP:
      report_in_.buttons0 = DS4::DPad::UP;
      break;
    case Gamepad::DPAD_UP_RIGHT:
      report_in_.buttons0 = DS4::DPad::UP_RIGHT;
      break;
    case Gamepad::DPAD_RIGHT:
      report_in_.buttons0 = DS4::DPad::RIGHT;
      break;
    case Gamepad::DPAD_DOWN_RIGHT:
      report_in_.buttons0 = DS4::DPad::DOWN_RIGHT;
      break;
    case Gamepad::DPAD_DOWN:
      report_in_.buttons0 = DS4::DPad::DOWN;
      break;
    case Gamepad::DPAD_DOWN_LEFT:
      report_in_.buttons0 = DS4::DPad::DOWN_LEFT;
      break;
    case Gamepad::DPAD_LEFT:
      report_in_.buttons0 = DS4::DPad::LEFT;
      break;
    case Gamepad::DPAD_UP_LEFT:
      report_in_.buttons0 = DS4::DPad::UP_LEFT;
      break;
    default:
      report_in_.buttons0 = DS4::DPad::CENTER;
      break;
    }

    // === Face buttons (upper nibble of buttons0) ===
    // Square, Cross, Circle, Triangle
    if (gp_in.buttons & Gamepad::BUTTON_X)
      report_in_.buttons0 |= DS4::Buttons0::SQUARE;
    if (gp_in.buttons & Gamepad::BUTTON_A)
      report_in_.buttons0 |= DS4::Buttons0::CROSS;
    if (gp_in.buttons & Gamepad::BUTTON_B)
      report_in_.buttons0 |= DS4::Buttons0::CIRCLE;
    if (gp_in.buttons & Gamepad::BUTTON_Y)
      report_in_.buttons0 |= DS4::Buttons0::TRIANGLE;

    // === Shoulder and menu buttons (buttons1) ===
    // L1, R1, L2(digital), R2(digital), Share, Options, L3, R3
    if (gp_in.buttons & Gamepad::BUTTON_LB)
      report_in_.buttons1 |= DS4::Buttons1::L1;
    if (gp_in.buttons & Gamepad::BUTTON_RB)
      report_in_.buttons1 |= DS4::Buttons1::R1;

    // L2/R2 digital bits - set when trigger is pressed at all
    if (gp_in.trigger_l > 0)
      report_in_.buttons1 |= DS4::Buttons1::L2;
    if (gp_in.trigger_r > 0)
      report_in_.buttons1 |= DS4::Buttons1::R2;

    // Share = BACK/SELECT button (CRITICAL for PS3!)
    if (gp_in.buttons & Gamepad::BUTTON_BACK)
      report_in_.buttons1 |= DS4::Buttons1::SHARE;
    // Options = START button
    if (gp_in.buttons & Gamepad::BUTTON_START)
      report_in_.buttons1 |= DS4::Buttons1::OPTIONS;

    // Stick clicks
    if (gp_in.buttons & Gamepad::BUTTON_L3)
      report_in_.buttons1 |= DS4::Buttons1::L3;
    if (gp_in.buttons & Gamepad::BUTTON_R3)
      report_in_.buttons1 |= DS4::Buttons1::R3;

    // === System buttons (buttons2) ===
    // PS button, Touchpad click, and 6-bit counter
    if (gp_in.buttons & Gamepad::BUTTON_SYS)
      report_in_.buttons2 |= DS4::Buttons2::PS;
    if (gp_in.buttons & Gamepad::BUTTON_MISC)
      report_in_.buttons2 |= DS4::Buttons2::TP;

    // Counter (6 bits, bits 2-7) - increments each report
    report_in_.buttons2 |= ((report_counter_++ & 0x3F) << 2);

    // === Analog triggers (CRITICAL for PS3 games!) ===
    // These are the analog values (0-255) that racing games need
    report_in_.trigger_l = gp_in.trigger_l;
    report_in_.trigger_r = gp_in.trigger_r;

    // === Sensor data (can be zeroed for basic functionality) ===
    // Some games may expect valid sensor data
    report_in_.timestamp = 0;
    report_in_.battery = 0x00;
    report_in_.battery_level = 0x0B; // Full battery, USB connected
  }

  if (tud_suspended()) {
    tud_remote_wakeup();
  }

  if (tud_hid_ready()) {
    // Send full 64-byte report including report_id
    tud_hid_report(0, reinterpret_cast<uint8_t *>(&report_in_),
                   sizeof(DS4::InReport));
  }

  if (new_report_out_) {
    Gamepad::PadOut gp_out;
    gp_out.rumble_l = report_out_.motor_left;
    gp_out.rumble_r = report_out_.motor_right;
    gamepad.set_pad_out(gp_out);
    new_report_out_ = false;
  }
}

uint16_t DS4Device::get_report_cb(uint8_t itf, uint8_t report_id,
                                  hid_report_type_t report_type,
                                  uint8_t *buffer, uint16_t reqlen) {
  if (report_type == HID_REPORT_TYPE_INPUT) {
    std::memcpy(buffer, &report_in_, sizeof(DS4::InReport));
    return sizeof(DS4::InReport);
  } else if (report_type == HID_REPORT_TYPE_FEATURE) {
    // Handle feature reports that PS3/PS4 might request
    // Return zeros for most feature reports
    std::memset(buffer, 0, reqlen);
    return reqlen;
  }
  return 0;
}

void DS4Device::set_report_cb(uint8_t itf, uint8_t report_id,
                              hid_report_type_t report_type,
                              uint8_t const *buffer, uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_OUTPUT) {
    // Output report for rumble/LED
    if (report_id == 0x05 ||
        (report_id == 0 && bufsize > 0 && buffer[0] == 0x05)) {
      new_report_out_ = true;
      const uint8_t *data = (report_id == 0) ? &buffer[1] : buffer;
      uint16_t size = (report_id == 0) ? bufsize - 1 : bufsize;
      std::memcpy(
          &report_out_, data,
          std::min(size, static_cast<uint16_t>(sizeof(DS4::OutReport))));
    }
  }
}

bool DS4Device::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                       tusb_control_request_t const *request) {
  return false;
}

const uint16_t *DS4Device::get_descriptor_string_cb(uint8_t index,
                                                    uint16_t langid) {
  const char *value =
      reinterpret_cast<const char *>(DS4::STRING_DESCRIPTORS[index]);
  return get_string_descriptor(value, index);
}

const uint8_t *DS4Device::get_descriptor_device_cb() {
  return DS4::DEVICE_DESCRIPTORS;
}

const uint8_t *DS4Device::get_hid_descriptor_report_cb(uint8_t itf) {
  return DS4::REPORT_DESCRIPTORS;
}

const uint8_t *DS4Device::get_descriptor_configuration_cb(uint8_t index) {
  return DS4::CONFIGURATION_DESCRIPTORS;
}

const uint8_t *DS4Device::get_descriptor_device_qualifier_cb() {
  return nullptr;
}
