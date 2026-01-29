#ifndef _PS3_GUITAR_HOST_H_
#define _PS3_GUITAR_HOST_H_

#include <array>
#include <cstdint>


#include "tusb.h"

#include "Descriptors/PS3Guitar.h"
#include "USBHost/HostDriver/HostDriver.h"

class PS3GuitarHost : public HostDriver {
public:
  PS3GuitarHost(uint8_t idx) : HostDriver(idx) {}

  void initialize(Gamepad &gamepad, uint8_t address, uint8_t instance,
                  const uint8_t *report_desc, uint16_t desc_len) override;
  void process_report(Gamepad &gamepad, uint8_t address, uint8_t instance,
                      const uint8_t *report, uint16_t len) override;
  bool send_feedback(Gamepad &gamepad, uint8_t address,
                     uint8_t instance) override;

private:
  enum class InitStage { ENABLE_REPORT, DONE };

  struct InitState {
    uint8_t dev_addr{0xFF};
    InitStage stage{InitStage::ENABLE_REPORT};
    std::array<uint8_t, 17> init_buffer; // Buffer for control transfers
  };

  PS3Guitar::InReport prev_in_report_;
  InitState init_state_;

  static bool send_control_xfer(uint8_t dev_addr,
                                const tusb_control_request_t *req,
                                uint8_t *buffer, tuh_xfer_cb_t complete_cb,
                                uintptr_t user_data);
  static void init_complete_cb(tuh_xfer_s *xfer);
};

#endif // _PS3_GUITAR_HOST_H_
