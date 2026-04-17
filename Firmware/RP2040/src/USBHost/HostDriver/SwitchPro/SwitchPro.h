#ifndef _SWITCH_PRO_HOST_H_
#define _SWITCH_PRO_HOST_H_

#include <array>
#include <cstdint>

#include "Board/ogxm_log.h"
#include "Descriptors/SwitchPro.h"
#include "USBHost/HostDriver/HostDriver.h"

class SwitchProHost : public HostDriver
{
public:
    SwitchProHost(uint8_t idx)
        : HostDriver(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;
    void report_sent_cb(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    void disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance) override;
    void get_report_complete_cb(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                uint8_t report_id, uint8_t report_type, uint16_t len) override;
    void set_report_complete_cb(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                uint8_t report_id, uint8_t report_type, uint16_t len) override;

private:
    enum class InitState
    {
        HANDSHAKE,
        DISABLE_TIMEOUT,
        SET_FULL_REPORT,
        WAIT_FULL_REPORT,
        ENABLE_VIBRATION,
        WAIT_VIBRATION,
        ENABLE_IMU,
        WAIT_IMU,
        SET_LED,
        WAIT_LED,
        SET_HOME_LED,
        WAIT_HOME_LED,
        READY_FULL,
        READY_COMPAT_RUMBLE,
        READY_COMPAT_INPUT,
    };

    enum class ControlFallbackState
    {
        IDLE,
        HANDSHAKE,
        DISABLE_TIMEOUT,
        SET_FULL_REPORT,
        DONE,
        FAILED,
    };

    enum class GetReportProbeState
    {
        IDLE,
        INPUT_0x30,
        INPUT_0x81,
        INPUT_0x21,
        DONE,
        FAILED,
    };

    static constexpr uint32_t INIT_RETRY_INTERVAL = 8;
    static constexpr uint32_t INIT_TIMEOUT_FULL_REPORT = 40;
    static constexpr uint32_t INIT_TIMEOUT_OPTIONAL = 16;
    static constexpr uint32_t CLONE_PRE_FULL_REPORT_HINT_WINDOW_MS = 20;
    static constexpr uint32_t CLONE_FULL_REPORT_SETTLE_DELAY_MS = 75;
    static constexpr uint32_t CLONE_READY_RECOVERY_KICK_DELAY_MS = 16;
    static constexpr uint32_t CLONE_READY_MODE_RETRY_DELAY_MS = 120;
    static constexpr uint32_t GET_REPORT_PROBE_TIMEOUT_MS = 180;
    static constexpr uint8_t MAX_DEBUG_INPUT_REPORT_LOGS = 24;
    static constexpr uint8_t MAX_VENDOR_STATUS_REPORT_LOGS = 12;
    static constexpr uint8_t MAX_GET_REPORT_PROBE_TIMEOUTS = 3;
    static constexpr uint8_t READY_KEEPALIVE_BURST = 8;
    static constexpr uint8_t CLONE_VENDOR_STATUS_SIGNATURE[8] = {
        0x81, 0x01, 0x00, 0x03, 0x42, 0x06, 0xF6, 0xC4
    };

    InitState init_state_{InitState::HANDSHAKE};
    uint8_t sequence_counter_{0};
    uint32_t init_timeout_count_{0};
    bool full_report_seen_{false};
    bool input_stream_seen_{false};
    bool rumble_capable_{false};
    bool imu_enabled_{false};
    bool using_compat_fallback_{false};
    bool clone_init_path_active_{false};
    bool saw_vendor_status_report_{false};
    bool clone_pre_full_report_hint_window_attempted_{false};
    bool clone_full_report_delay_logged_{false};
    bool clone_ready_mode_retry_attempted_{false};
    bool clone_ready_mode_retry_wait_logged_{false};
    bool clone_post_ready_reinit_attempted_{false};
    bool control_fallback_attempted_{false};
    bool control_fallback_active_{false};
    bool get_report_probe_attempted_{false};
    bool get_report_probe_active_{false};
    uint8_t debug_input_report_logs_{0};
    uint8_t vendor_status_report_logs_{0};
    uint8_t ready_keepalive_budget_{0};
    uint32_t clone_pre_full_report_hint_ready_at_ms_{0};
    uint32_t clone_full_report_ready_at_ms_{0};
    uint32_t clone_ready_mode_retry_at_ms_{0};
    uint32_t get_report_probe_started_at_ms_{0};
    uint32_t clone_init_reentry_task_id_{0};
    uint8_t get_report_probe_timeout_count_{0};
    ControlFallbackState control_fallback_state_{ControlFallbackState::IDLE};
    GetReportProbeState get_report_probe_state_{GetReportProbeState::IDLE};

    SwitchPro::InReport prev_in_report_{};
    SwitchPro::OutReport out_report_{};
    SwitchPro::HidCommand control_hid_command_{};
    SwitchPro::OutReport control_out_report_{};
    std::array<uint8_t, 64> control_get_report_buffer_{};

    static const char* init_state_name(InitState state);
    static const char* control_fallback_state_name(ControlFallbackState state);
    static const char* get_report_probe_state_name(GetReportProbeState state);
    void set_init_state(InitState state, const char* reason);
    void activate_parallel_clone_path(const char* reason);
    bool is_clone_vendor_status_signature(const uint8_t* report, uint16_t len) const;
    void schedule_clone_full_report_settle_delay(uint8_t address, uint8_t instance, const char* reason);
    void schedule_clone_init_reentry(uint8_t address, uint8_t instance, uint32_t delay_ms);
    void cancel_clone_init_reentry();
    bool should_prioritize_clone_reports() const;
    bool waiting_for_clone_post_ready_mode_retry_result() const;
    void reset_state();
    void init_switch_host(uint8_t address, uint8_t instance);
    void update_init_state(uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len);
    void handle_timeout();
    void advance_after_full_report();
    void advance_after_vibration();
    void advance_after_imu();
    void advance_after_led();
    void advance_after_home_led(uint8_t address, uint8_t instance);
    void observe_vendor_status_report(uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len);
    void mark_input_stream_seen(uint8_t report_id);
    bool maybe_send_clone_post_ready_mode_retry(uint8_t address, uint8_t instance, const char* reason);
    bool maybe_advance_clone_post_ready_recovery(uint8_t address, uint8_t instance, const char* reason);
    bool maybe_start_clone_post_ready_reinit(uint8_t address, uint8_t instance, const char* reason);
    void maybe_start_control_fallback(uint8_t address, uint8_t instance, const char* reason);
    void maybe_start_get_report_probe(uint8_t address, uint8_t instance, const char* reason);
    bool handle_get_report_probe_timeout(uint8_t address, uint8_t instance);
    void advance_control_fallback(uint8_t address, uint8_t instance, bool success,
                                  uint8_t report_id, uint8_t report_type, uint16_t len);
    void advance_get_report_probe(uint8_t address, uint8_t instance, bool success,
                                  uint8_t report_id, uint8_t report_type, uint16_t len);
    bool queue_control_fallback_step(uint8_t address, uint8_t instance);
    bool queue_get_report_probe_step(uint8_t address, uint8_t instance);

    bool send_hid_command(uint8_t address, uint8_t instance, uint8_t command);
    bool send_subcommand(uint8_t address, uint8_t instance, uint8_t sub_command,
                         const uint8_t* data = nullptr, uint8_t data_len = 0);
    bool send_keepalive_rumble(uint8_t address, uint8_t instance, const char* reason);
    bool send_control_hid_command(uint8_t address, uint8_t instance, uint8_t command);
    bool send_control_subcommand(uint8_t address, uint8_t instance, uint8_t sub_command,
                                 const uint8_t* data = nullptr, uint8_t data_len = 0);
    bool send_control_get_report(uint8_t address, uint8_t instance, uint8_t report_id,
                                 uint8_t report_type, uint16_t len);
    uint8_t get_output_sequence_counter();

    bool is_ready() const;
    bool parse_switch_wired_report(Gamepad& gamepad, const uint8_t* report, uint16_t len);
    bool parse_switch_button_report(Gamepad& gamepad, const uint8_t* report, uint16_t len);
    bool parse_switch_full_report(Gamepad& gamepad, const uint8_t* report, uint16_t len);

    static inline int16_t normalize_axis(uint16_t value)
    {
        /*  12bit value from the controller doesnt cover the full 12bit range seemingly,
            isn't completely centered at 2047 either so I may be missing something here.
            Tried to get as close as possible with the multiplier */
        int32_t normalized_value = (value - 2047) * 22;
        return Range::clamp<int16_t>(normalized_value);
    }
};

#endif // _SWITCH_PRO_HOST_H_
