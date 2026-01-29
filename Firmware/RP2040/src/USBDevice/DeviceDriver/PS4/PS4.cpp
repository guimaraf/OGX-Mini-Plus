#include <algorithm>
#include <cstring>

#include "USBDevice/DeviceDriver/PS4/PS4.h"

void PS4Device::initialize() {
  class_driver_ = {.name = TUD_DRV_NAME("PS4"),
                   .init = hidd_init,
                   .deinit = hidd_deinit,
                   .reset = hidd_reset,
                   .open = hidd_open,
                   .control_xfer_cb = hidd_control_xfer_cb,
                   .xfer_cb = hidd_xfer_cb,
                   .sof = NULL};
}

void PS4Device::process(const uint8_t idx, Gamepad &gamepad) {
  if (gamepad.new_pad_in()) {
    Gamepad::PadIn gp_in = gamepad.get_pad_in();

    // Inicializar report com zeros e valores padrão
    std::memset(&report_in_, 0, sizeof(PS4::InReport));
    report_in_.report_id = 0x01;

    // Joysticks - PS4 usa 0x80 (128) como centro
    constexpr int16_t DEADZONE = 1640; // ~5%
    constexpr uint8_t PS4_CENTER = 0x80;

    auto joystick_to_ps4 = [](int16_t value, int16_t deadzone) -> uint8_t {
      if (value > -deadzone && value < deadzone) {
        return PS4_CENTER;
      }
      // Converter int16 (-32768 a +32767) para uint8 (0 a 255)
      int32_t scaled = ((static_cast<int32_t>(value) + 32768) * 255) / 65535;
      return static_cast<uint8_t>(scaled);
    };

    report_in_.joystick_lx = joystick_to_ps4(gp_in.joystick_lx, DEADZONE);
    report_in_.joystick_ly = joystick_to_ps4(gp_in.joystick_ly, DEADZONE);
    report_in_.joystick_rx = joystick_to_ps4(gp_in.joystick_rx, DEADZONE);
    report_in_.joystick_ry = joystick_to_ps4(gp_in.joystick_ry, DEADZONE);

    // D-pad usando HAT switch (0-7 para direções, 8 para centro)
    switch (gp_in.dpad) {
    case Gamepad::DPAD_UP:
      report_in_.buttons0 = PS4::Buttons0::DPAD_UP;
      break;
    case Gamepad::DPAD_UP_RIGHT:
      report_in_.buttons0 = PS4::Buttons0::DPAD_UP_RIGHT;
      break;
    case Gamepad::DPAD_RIGHT:
      report_in_.buttons0 = PS4::Buttons0::DPAD_RIGHT;
      break;
    case Gamepad::DPAD_DOWN_RIGHT:
      report_in_.buttons0 = PS4::Buttons0::DPAD_RIGHT_DOWN;
      break;
    case Gamepad::DPAD_DOWN:
      report_in_.buttons0 = PS4::Buttons0::DPAD_DOWN;
      break;
    case Gamepad::DPAD_DOWN_LEFT:
      report_in_.buttons0 = PS4::Buttons0::DPAD_DOWN_LEFT;
      break;
    case Gamepad::DPAD_LEFT:
      report_in_.buttons0 = PS4::Buttons0::DPAD_LEFT;
      break;
    case Gamepad::DPAD_UP_LEFT:
      report_in_.buttons0 = PS4::Buttons0::DPAD_LEFT_UP;
      break;
    default:
      report_in_.buttons0 = PS4::Buttons0::DPAD_CENTER;
      break;
    }

    // Face buttons - Square, Cross, Circle, Triangle (bits 4-7 de buttons0)
    if (gp_in.buttons & Gamepad::BUTTON_X)
      report_in_.buttons0 |= PS4::Buttons0::SQUARE;
    if (gp_in.buttons & Gamepad::BUTTON_A)
      report_in_.buttons0 |= PS4::Buttons0::CROSS;
    if (gp_in.buttons & Gamepad::BUTTON_B)
      report_in_.buttons0 |= PS4::Buttons0::CIRCLE;
    if (gp_in.buttons & Gamepad::BUTTON_Y)
      report_in_.buttons0 |= PS4::Buttons0::TRIANGLE;

    // Shoulder buttons - L1, R1, L2, R2, Share, Options, L3, R3
    if (gp_in.buttons & Gamepad::BUTTON_LB)
      report_in_.buttons1 |= PS4::Buttons1::L1;
    if (gp_in.buttons & Gamepad::BUTTON_RB)
      report_in_.buttons1 |= PS4::Buttons1::R1;
    if (gp_in.trigger_l > 0)
      report_in_.buttons1 |= PS4::Buttons1::L2;
    if (gp_in.trigger_r > 0)
      report_in_.buttons1 |= PS4::Buttons1::R2;
    if (gp_in.buttons & Gamepad::BUTTON_BACK)
      report_in_.buttons1 |= PS4::Buttons1::SHARE;
    if (gp_in.buttons & Gamepad::BUTTON_START)
      report_in_.buttons1 |= PS4::Buttons1::OPTIONS;
    if (gp_in.buttons & Gamepad::BUTTON_L3)
      report_in_.buttons1 |= PS4::Buttons1::L3;
    if (gp_in.buttons & Gamepad::BUTTON_R3)
      report_in_.buttons1 |= PS4::Buttons1::R3;

    // System buttons - PS, Touchpad
    if (gp_in.buttons & Gamepad::BUTTON_SYS)
      report_in_.buttons2 |= PS4::Buttons2::PS;
    if (gp_in.buttons & Gamepad::BUTTON_MISC)
      report_in_.buttons2 |= PS4::Buttons2::TP;

    // Triggers analógicos (bytes 8-9 - CRÍTICO para jogos!)
    report_in_.trigger_l = gp_in.trigger_l;
    report_in_.trigger_r = gp_in.trigger_r;

    // Battery level (para alguns jogos/sistemas)
    report_in_.battery_level = 0xFF; // Bateria cheia
  }

  if (tud_suspended()) {
    tud_remote_wakeup();
  }

  if (tud_hid_ready()) {
    // Enviar report incluindo report_id no buffer (como PS3 faz)
    tud_hid_report(0, reinterpret_cast<uint8_t *>(&report_in_),
                   sizeof(PS4::InReport));
  }

  if (new_report_out_) {
    Gamepad::PadOut gp_out;
    gp_out.rumble_l = report_out_.motor_left;
    gp_out.rumble_r = report_out_.motor_right;
    gamepad.set_pad_out(gp_out);
    new_report_out_ = false;
  }
}

uint16_t PS4Device::get_report_cb(uint8_t itf, uint8_t report_id,
                                  hid_report_type_t report_type,
                                  uint8_t *buffer, uint16_t reqlen) {
  if (report_type == HID_REPORT_TYPE_INPUT) {
    std::memcpy(buffer, &report_in_, sizeof(PS4::InReport));
    return sizeof(PS4::InReport);
  }
  return 0;
}

void PS4Device::set_report_cb(uint8_t itf, uint8_t report_id,
                              hid_report_type_t report_type,
                              uint8_t const *buffer, uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_OUTPUT) {
    new_report_out_ = true;
    std::memcpy(
        &report_out_, buffer,
        std::min(bufsize, static_cast<uint16_t>(sizeof(PS4::OutReport))));
  }
}

bool PS4Device::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                       tusb_control_request_t const *request) {
  return false;
}

const uint16_t *PS4Device::get_descriptor_string_cb(uint8_t index,
                                                    uint16_t langid) {
  const char *value =
      reinterpret_cast<const char *>(PS4::STRING_DESCRIPTORS[index]);
  return get_string_descriptor(value, index);
}

const uint8_t *PS4Device::get_descriptor_device_cb() {
  return PS4::DEVICE_DESCRIPTORS;
}

const uint8_t *PS4Device::get_hid_descriptor_report_cb(uint8_t itf) {
  return PS4::REPORT_DESCRIPTORS;
}

const uint8_t *PS4Device::get_descriptor_configuration_cb(uint8_t index) {
  return PS4::CONFIGURATION_DESCRIPTORS;
}

const uint8_t *PS4Device::get_descriptor_device_qualifier_cb() {
  return nullptr;
}
