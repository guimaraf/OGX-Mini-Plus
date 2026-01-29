#include <algorithm>
#include <cstring>


#include "tusb.h"

#include "USBHost/HostDriver/PS3Guitar/PS3Guitar.h"

// Initialization logic to enable reports (Magic Packet for PS3 controllers)
void PS3GuitarHost::initialize(Gamepad &gamepad, uint8_t address,
                               uint8_t instance, const uint8_t *report_desc,
                               uint16_t desc_len) {
  gamepad.set_analog_host(true);

  init_state_.dev_addr = address;
  init_state_.init_buffer.fill(0);

  // Standard PS3 initialization command (GET_REPORT Feature 0xF2)
  // This wakes up the controller/guitar and makes it start sending input
  // reports
  tusb_control_request_t init_request = {
      .bmRequestType = 0xA1, // Device-to-Host, Class, Interface
      .bRequest = 0x01,      // GET_REPORT
      .wValue = (HID_REPORT_TYPE_FEATURE << 8) | 0xF2,
      .wIndex = 0x0000,
      .wLength = 17};

  send_control_xfer(address, &init_request, init_state_.init_buffer.data(),
                    init_complete_cb,
                    reinterpret_cast<uintptr_t>(&init_state_));

  tuh_hid_receive_report(address, instance);
}

bool PS3GuitarHost::send_control_xfer(uint8_t dev_addr,
                                      const tusb_control_request_t *request,
                                      uint8_t *buffer,
                                      tuh_xfer_cb_t complete_cb,
                                      uintptr_t user_data) {
  tuh_xfer_s transfer = {.daddr = dev_addr,
                         .ep_addr = 0x00,
                         .setup = request,
                         .buffer = buffer,
                         .complete_cb = complete_cb,
                         .user_data = user_data};
  return tuh_control_xfer(&transfer);
}

void PS3GuitarHost::init_complete_cb(tuh_xfer_s *xfer) {
  InitState *init_state = reinterpret_cast<InitState *>(xfer->user_data);
  if (init_state) {
    init_state->stage = InitStage::DONE;
  }
}

void PS3GuitarHost::process_report(Gamepad &gamepad, uint8_t address,
                                   uint8_t instance, const uint8_t *report,
                                   uint16_t len) {
  // Check if report changed to avoid processing identical data
  // Only check first 44 bytes (buttons + sensors) to save time
  if (std::memcmp(
          &prev_in_report_, report,
          std::min(static_cast<size_t>(len), static_cast<size_t>(44))) == 0) {
    tuh_hid_receive_report(address, instance);
    return;
  }

  // Save report for next comparison
  std::memcpy(&prev_in_report_, report,
              std::min(static_cast<size_t>(len), sizeof(PS3Guitar::InReport)));

  // IMPORTANT: check if report has Report ID byte
  // Usually PS3 controllers send Report ID 0x01 as the first byte
  // So the data structure matches directly.
  const PS3Guitar::InReport *in_report =
      reinterpret_cast<const PS3Guitar::InReport *>(report);

  Gamepad::PadIn gp_in;

  // --- Button Mapping ---
  // PS3 Guitar -> Generic Gamepad -> XInput Guitar

  // Frets
  if (in_report->buttons1 & PS3Guitar::Buttons1::GREEN)
    gp_in.buttons |= gamepad.MAP_BUTTON_A; // Green -> A
  if (in_report->buttons1 & PS3Guitar::Buttons1::RED)
    gp_in.buttons |= gamepad.MAP_BUTTON_B; // Red -> B
  if (in_report->buttons1 & PS3Guitar::Buttons1::YELLOW)
    gp_in.buttons |= gamepad.MAP_BUTTON_Y; // Yellow -> Y
  if (in_report->buttons1 & PS3Guitar::Buttons1::BLUE)
    gp_in.buttons |= gamepad.MAP_BUTTON_X; // Blue -> X
  if (in_report->buttons1 & PS3Guitar::Buttons1::ORANGE)
    gp_in.buttons |= gamepad.MAP_BUTTON_LB; // Orange -> LB

  // Strum Bar (mapped to D-Pad Up/Down)
  if (in_report->buttons0 & PS3Guitar::Buttons0::STRUM_UP)
    gp_in.dpad |= gamepad.MAP_DPAD_UP;
  if (in_report->buttons0 & PS3Guitar::Buttons0::STRUM_DOWN)
    gp_in.dpad |= gamepad.MAP_DPAD_DOWN;

  // Other Buttons
  if (in_report->buttons0 & PS3Guitar::Buttons0::START)
    gp_in.buttons |= gamepad.MAP_BUTTON_START;
  if (in_report->buttons0 & PS3Guitar::Buttons0::SELECT)
    gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
  if (in_report->buttons2 & PS3Guitar::Buttons2::PS)
    gp_in.buttons |= gamepad.MAP_BUTTON_SYS; // Xbox Guide

  // Whammy Bar
  // Usually on Joystick RX (right stick X) or sometimes Z/Rz
  // We map it to Right Stick X for the XInput driver to pick up
  gp_in.joystick_rx =
      static_cast<int16_t>((in_report->joystick_rx - 128) * 256);

  // Joystick (some guitars have a small joystick)
  gp_in.joystick_lx =
      static_cast<int16_t>((in_report->joystick_lx - 128) * 256);
  gp_in.joystick_ly =
      static_cast<int16_t>((in_report->joystick_ly - 128) * 256);

  // --- TILT SENSOR IMPLEMENTATION ---

  // Read accelerometer X (Big Endian)
  // Bytes 41-42 in the struct (indices 41, 42)
  // Note: If report array starts without Report ID, indices would shift by -1.
  // Assuming standard report with ID at byte 0.

  // Safety check for length
  if (len >= 43) {
    // Standard PS3 Big Endian format
    // in_report->accel_x is uint16_t but aligned, so we might need manual
    // reconstruction if alignment is off Using manual byte access to be safe
    // against packing/endianness issues
    uint16_t raw_accel_x = (uint16_t(report[41]) << 8) | report[42];

    // Convert to 10-bit range (0-1023)
    // PS3 reports are 10-bit, usually left-aligned in 16-bit or just high
    // bytes? Actually PS3 accel is 10-bit: 0x0180 (384) to 0x0280 (640), rest
    // ~512 (0x0200)

    // Store directly in Gamepad::PadIn
    // The XInputGuitar360 driver expects a 10-bit value in accel_x
    gp_in.accel_x = static_cast<int16_t>(raw_accel_x);
  } else {
    gp_in.accel_x = 512; // Default center
  }

  gamepad.set_pad_in(gp_in);

  tuh_hid_receive_report(address, instance);
}

bool PS3GuitarHost::send_feedback(Gamepad &gamepad, uint8_t address,
                                  uint8_t instance) {
  // Guitar Hero guitars don't typically have rumble
  // But we could implement LED control here if needed
  return true;
}
