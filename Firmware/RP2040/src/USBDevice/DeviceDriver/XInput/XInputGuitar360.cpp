#include <cstring>

#include "USBDevice/DeviceDriver/XInput/XInputGuitar360.h"
#include "USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.h"

void XInputGuitar360Device::initialize() {
  class_driver_ = *tud_xinput::class_driver();
}

void XInputGuitar360Device::process(const uint8_t idx, Gamepad &gamepad) {
  if (gamepad.new_pad_in()) {
    in_report_.buttons[0] = 0;
    in_report_.buttons[1] = 0;
    in_report_.trigger_l = 0;
    in_report_.trigger_r = 0;

    Gamepad::PadIn gp_in = gamepad.get_pad_in();

    // Strum mapping: D-Pad Up/Down -> Strum Up/Down
    if (gp_in.dpad & Gamepad::DPAD_UP)
      in_report_.buttons[0] |= XInputGuitar360::Buttons0::DPAD_UP;
    if (gp_in.dpad & Gamepad::DPAD_DOWN)
      in_report_.buttons[0] |= XInputGuitar360::Buttons0::DPAD_DOWN;
    if (gp_in.dpad & Gamepad::DPAD_LEFT)
      in_report_.buttons[0] |= XInputGuitar360::Buttons0::DPAD_LEFT;
    if (gp_in.dpad & Gamepad::DPAD_RIGHT)
      in_report_.buttons[0] |= XInputGuitar360::Buttons0::DPAD_RIGHT;

    // Control buttons
    // Select (PS3 Btn 9) -> Back (Xbox)
    if (gp_in.buttons & Gamepad::BUTTON_BACK)
      in_report_.buttons[0] |= XInputGuitar360::Buttons0::BACK;
    // Start (PS3 Btn 10) -> Start (Xbox)
    if (gp_in.buttons & Gamepad::BUTTON_START)
      in_report_.buttons[0] |= XInputGuitar360::Buttons0::START;
    // PS Button -> Guide (Xbox)
    if (gp_in.buttons & Gamepad::BUTTON_SYS)
      in_report_.buttons[1] |= XInputGuitar360::Buttons1::HOME;

    // Guitar Fret Mapping (Corrected for Rock Band / Guitar Hero)
    // PS3 Green  (Btn 2/A)  -> Xbox Green (A)
    // PS3 Red    (Btn 3/B)  -> Xbox Red (B)
    // PS3 Yellow (Btn 1/X)  -> Xbox Yellow (Y)
    // PS3 Blue   (Btn 4/Y)  -> Xbox Blue (X)
    // PS3 Orange (Btn 5/LB) -> Xbox Orange (LB) OR RB? Standard is LB for
    // Orange.

    // Note: HIDGeneric maps:
    // Btn 1 -> X (Yellow input)
    // Btn 2 -> A (Green input)
    // Btn 3 -> B (Red input)
    // Btn 4 -> Y (Blue input)
    // Btn 5 -> LB (Orange input)

    if (gp_in.buttons & Gamepad::BUTTON_A)
      in_report_.buttons[1] |= XInputGuitar360::Buttons1::A; // Green -> A
    if (gp_in.buttons & Gamepad::BUTTON_B)
      in_report_.buttons[1] |= XInputGuitar360::Buttons1::B; // Red -> B
    if (gp_in.buttons & Gamepad::BUTTON_X)
      in_report_.buttons[1] |=
          XInputGuitar360::Buttons1::Y; // Yellow (In: X) -> Out: Y
    if (gp_in.buttons & Gamepad::BUTTON_Y)
      in_report_.buttons[1] |=
          XInputGuitar360::Buttons1::X; // Blue (In: Y) -> Out: X
    if (gp_in.buttons & Gamepad::BUTTON_LB)
      in_report_.buttons[1] |= XInputGuitar360::Buttons1::LB; // Orange -> LB

    // Whammy bar
    // User: "Distortion (Whammy) = Z Axis increasing"
    // HIDGeneric maps Z axis to joystick_rx.
    // XInput Guitar uses RX for Whammy.
    in_report_.joystick_lx = gp_in.joystick_lx;

    // Tilt sensor: Convert PS3 accelerometer X to Xbox 360 Left Stick Y
    // PS3 guitar tilt: accel_x < 400 = tilted up (from user testing: 388 when
    // tilted) Xbox 360 guitar: Left Stick Y = INT16_MAX (32767) when tilted
    // This triggers Star Power/Overdrive in Rock Band and Guitar Hero games
    constexpr int16_t TILT_THRESHOLD = 400;
    if (gp_in.accel_x < TILT_THRESHOLD && gp_in.accel_x > 0) {
      in_report_.joystick_ly =
          INT16_MAX; // Tilt activated - full positive deflection
    } else {
      in_report_.joystick_ly = 0; // No tilt - center position
    }

    // Whammy bar fix: Apply deadzone to filter out resting state
    // The PS3 guitar whammy bar may send non-zero values even at rest.
    // Xbox 360 expects whammy at rest = 0, pressed = positive values.
    // Apply a deadzone threshold to filter noise when whammy is not pressed.
    constexpr int16_t WHAMMY_DEADZONE = 4000; // ~12% of full range as deadzone
    if (gp_in.joystick_rx > WHAMMY_DEADZONE) {
      // Whammy is pressed - map to full range
      in_report_.joystick_rx = gp_in.joystick_rx;
    } else if (gp_in.joystick_rx < -WHAMMY_DEADZONE) {
      // Handle negative values (inverted whammy)
      in_report_.joystick_rx = gp_in.joystick_rx;
    } else {
      // Within deadzone - whammy at rest, send zero
      in_report_.joystick_rx = 0;
    }
    in_report_.joystick_ry = Range::invert(gp_in.joystick_ry);

    if (tud_suspended()) {
      tud_remote_wakeup();
    }

    tud_xinput::send_report((uint8_t *)&in_report_,
                            sizeof(XInputGuitar360::InReport));
  }

  if (tud_xinput::receive_report(reinterpret_cast<uint8_t *>(&out_report_),
                                 sizeof(XInputGuitar360::OutReport)) &&
      out_report_.report_id == 0x00) {
    Gamepad::PadOut gp_out;
    gp_out.rumble_l = out_report_.rumble_l;
    gp_out.rumble_r = out_report_.rumble_r;
    gamepad.set_pad_out(gp_out);
  }
}

uint16_t XInputGuitar360Device::get_report_cb(uint8_t itf, uint8_t report_id,
                                              hid_report_type_t report_type,
                                              uint8_t *buffer,
                                              uint16_t reqlen) {
  std::memcpy(buffer, &in_report_, sizeof(XInputGuitar360::InReport));
  return sizeof(XInputGuitar360::InReport);
}

void XInputGuitar360Device::set_report_cb(uint8_t itf, uint8_t report_id,
                                          hid_report_type_t report_type,
                                          const uint8_t *buffer,
                                          uint16_t bufsize) {}

bool XInputGuitar360Device::vendor_control_xfer_cb(
    uint8_t rhport, uint8_t stage, const tusb_control_request_t *request) {
  return false;
}

const uint16_t *
XInputGuitar360Device::get_descriptor_string_cb(uint8_t index,
                                                uint16_t langid) {
  const char *value =
      reinterpret_cast<const char *>(XInputGuitar360::DESC_STRING[index]);
  return get_string_descriptor(value, index);
}

const uint8_t *XInputGuitar360Device::get_descriptor_device_cb() {
  return XInputGuitar360::DESC_DEVICE;
}

const uint8_t *
XInputGuitar360Device::get_hid_descriptor_report_cb(uint8_t itf) {
  return nullptr;
}

const uint8_t *
XInputGuitar360Device::get_descriptor_configuration_cb(uint8_t index) {
  return XInputGuitar360::DESC_CONFIGURATION;
}

const uint8_t *XInputGuitar360Device::get_descriptor_device_qualifier_cb() {
  return nullptr;
}
