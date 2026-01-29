#ifndef _DS4_DEVICE_H_
#define _DS4_DEVICE_H_

#include <cstdint>

#include "Descriptors/DS4.h"
#include "USBDevice/DeviceDriver/DeviceDriver.h"

// DualShock 4 Device Driver
// Emulates authentic Sony DualShock 4 V2 (VID: 054C, PID: 09CC)
// Compatible with PS3 (native) and potentially PS4/PS5

class DS4Device : public DeviceDriver {
public:
  void initialize() override;
  void process(const uint8_t idx, Gamepad &gamepad) override;
  uint16_t get_report_cb(uint8_t itf, uint8_t report_id,
                         hid_report_type_t report_type, uint8_t *buffer,
                         uint16_t reqlen) override;
  void set_report_cb(uint8_t itf, uint8_t report_id,
                     hid_report_type_t report_type, uint8_t const *buffer,
                     uint16_t bufsize) override;
  bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                              tusb_control_request_t const *request) override;
  const uint16_t *get_descriptor_string_cb(uint8_t index,
                                           uint16_t langid) override;
  const uint8_t *get_descriptor_device_cb() override;
  const uint8_t *get_hid_descriptor_report_cb(uint8_t itf) override;
  const uint8_t *get_descriptor_configuration_cb(uint8_t index) override;
  const uint8_t *get_descriptor_device_qualifier_cb() override;

private:
  DS4::InReport report_in_;
  DS4::OutReport report_out_;
  bool new_report_out_{false};
  uint8_t report_counter_{0}; // Counter for buttons2 field
};

#endif // _DS4_DEVICE_H_
