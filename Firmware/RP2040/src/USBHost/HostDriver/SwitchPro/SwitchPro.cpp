#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "Board/board_api.h"
#include "Descriptors/SwitchWired.h"
#include "USBHost/HostDriver/SwitchPro/SwitchPro.h"
#include "USBHost/HostDriver/SwitchPro/SwitchProCloneRecovery.h"
#include "USBHost/HostDriver/SwitchPro/SwitchProRumble.h"

const char* SwitchProHost::init_state_name(InitState state)
{
    switch (state)
    {
        case InitState::HANDSHAKE:           return "HANDSHAKE";
        case InitState::DISABLE_TIMEOUT:     return "DISABLE_TIMEOUT";
        case InitState::SET_FULL_REPORT:     return "SET_FULL_REPORT";
        case InitState::WAIT_FULL_REPORT:    return "WAIT_FULL_REPORT";
        case InitState::ENABLE_VIBRATION:    return "ENABLE_VIBRATION";
        case InitState::WAIT_VIBRATION:      return "WAIT_VIBRATION";
        case InitState::ENABLE_IMU:          return "ENABLE_IMU";
        case InitState::WAIT_IMU:            return "WAIT_IMU";
        case InitState::SET_LED:             return "SET_LED";
        case InitState::WAIT_LED:            return "WAIT_LED";
        case InitState::SET_HOME_LED:        return "SET_HOME_LED";
        case InitState::WAIT_HOME_LED:       return "WAIT_HOME_LED";
        case InitState::READY_FULL:          return "READY_FULL";
        case InitState::READY_COMPAT_RUMBLE: return "READY_COMPAT_RUMBLE";
        case InitState::READY_COMPAT_INPUT:  return "READY_COMPAT_INPUT";
        default:                             return "UNKNOWN";
    }
}

const char* SwitchProHost::control_fallback_state_name(ControlFallbackState state)
{
    switch (state)
    {
        case ControlFallbackState::IDLE:              return "IDLE";
        case ControlFallbackState::HANDSHAKE:         return "HANDSHAKE";
        case ControlFallbackState::DISABLE_TIMEOUT:   return "DISABLE_TIMEOUT";
        case ControlFallbackState::SET_FULL_REPORT:   return "SET_FULL_REPORT";
        case ControlFallbackState::ENABLE_VIBRATION:  return "ENABLE_VIBRATION";
        case ControlFallbackState::ENABLE_IMU:        return "ENABLE_IMU";
        case ControlFallbackState::SET_LED:           return "SET_LED";
        case ControlFallbackState::SET_HOME_LED:      return "SET_HOME_LED";
        case ControlFallbackState::DONE:              return "DONE";
        case ControlFallbackState::FAILED:            return "FAILED";
        default:                                      return "UNKNOWN";
    }
}

const char* SwitchProHost::get_report_probe_state_name(GetReportProbeState state)
{
    switch (state)
    {
        case GetReportProbeState::IDLE:      return "IDLE";
        case GetReportProbeState::INPUT_0x30:return "INPUT_0x30";
        case GetReportProbeState::INPUT_0x81:return "INPUT_0x81";
        case GetReportProbeState::INPUT_0x21:return "INPUT_0x21";
        case GetReportProbeState::DONE:      return "DONE";
        case GetReportProbeState::FAILED:    return "FAILED";
        default:                             return "UNKNOWN";
    }
}

void SwitchProHost::set_init_state(InitState state, const char* reason)
{
    if (init_state_ != state)
    {
        OGXM_LOG("SwitchProHost[%u]: state %s -> %s (%s)\n",
                 idx_,
                 init_state_name(init_state_),
                 init_state_name(state),
                 (reason != nullptr) ? reason : "no reason");
    }
    init_state_ = state;
}

void SwitchProHost::activate_parallel_clone_path(const char* reason)
{
    if (clone_init_path_active_)
    {
        return;
    }

    clone_init_path_active_ = true;
    SwitchProCloneRecovery::mark_clone_profile_detected();
    OGXM_LOG("SwitchProHost[%u]: activating ParallelClone init path (%s)\n",
             idx_, (reason != nullptr) ? reason : "no reason");
}

void SwitchProHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    (void)gamepad;
    (void)report_desc;
    (void)desc_len;

    OGXM_LOG("SwitchProHost[%u]: initialize addr=%u inst=%u desc_len=%u\n",
             idx_, address, instance, desc_len);
    reset_state();
    init_switch_host(address, instance);
    tuh_hid_receive_report(address, instance);
}

void SwitchProHost::reset_state()
{
    init_state_ = InitState::HANDSHAKE;
    sequence_counter_ = 0;
    init_timeout_count_ = 0;
    full_report_seen_ = false;
    input_stream_seen_ = false;
    rumble_capable_ = false;
    imu_enabled_ = false;
    using_compat_fallback_ = false;
    clone_init_path_active_ = false;
    saw_vendor_status_report_ = false;
    clone_full_report_delay_logged_ = false;
    clone_ready_mode_retry_pending_ = false;
    clone_ready_mode_retry_attempted_ = false;
    control_fallback_attempted_ = false;
    control_fallback_active_ = false;
    get_report_probe_attempted_ = false;
    get_report_probe_active_ = false;
    debug_input_report_logs_ = 0;
    vendor_status_report_logs_ = 0;
    ready_keepalive_budget_ = 0;
    clone_full_report_ready_at_ms_ = 0;
    clone_ready_mode_retry_at_ms_ = 0;
    get_report_probe_started_at_ms_ = 0;
    get_report_probe_timeout_count_ = 0;
    control_fallback_state_ = ControlFallbackState::IDLE;
    get_report_probe_state_ = GetReportProbeState::IDLE;
    std::memset(&prev_in_report_, 0, sizeof(prev_in_report_));
    std::memset(&out_report_, 0, sizeof(out_report_));
    std::memset(&control_hid_command_, 0, sizeof(control_hid_command_));
    std::memset(&control_out_report_, 0, sizeof(control_out_report_));
    control_get_report_buffer_.fill(0);

    OGXM_LOG("SwitchProHost[%u]: reset_state -> %s\n", idx_, init_state_name(init_state_));
}

bool SwitchProHost::is_ready() const
{
    switch (init_state_)
    {
        case InitState::READY_FULL:
        case InitState::READY_COMPAT_RUMBLE:
        case InitState::READY_COMPAT_INPUT:
            return true;
        default:
            return false;
    }
}

uint8_t SwitchProHost::get_output_sequence_counter()
{
    uint8_t counter = sequence_counter_;
    sequence_counter_ = (sequence_counter_ + 1) & 0x0F;
    return counter;
}

bool SwitchProHost::send_hid_command(uint8_t address, uint8_t instance, uint8_t command)
{
    const SwitchPro::HidCommand hid_command{
        .report_id = SwitchPro::REPORT::OUTPUT_HID,
        .command = command,
    };

    const bool ok = tuh_hid_send_report(address, instance, 0, &hid_command, sizeof(hid_command));
    OGXM_LOG("SwitchProHost[%u]: send_hid_command addr=%u inst=%u cmd=0x%02X ok=%u\n",
             idx_, address, instance, command, ok ? 1 : 0);
    return ok;
}

bool SwitchProHost::send_subcommand(uint8_t address, uint8_t instance, uint8_t sub_command,
                                    const uint8_t* data, uint8_t data_len)
{
    std::memset(&out_report_, 0, sizeof(out_report_));

    out_report_.command = SwitchPro::REPORT::OUTPUT_SUBCMD;
    out_report_.sequence_counter = get_output_sequence_counter();
    SwitchProRumble::fill_neutral(out_report_.rumble_l);
    SwitchProRumble::fill_neutral(out_report_.rumble_r);
    out_report_.sub_command = sub_command;

    if (data != nullptr && data_len > 0)
    {
        if (data_len > sizeof(out_report_.sub_command_args))
        {
            data_len = sizeof(out_report_.sub_command_args);
        }
        std::memcpy(out_report_.sub_command_args, data, data_len);
    }

    const uint8_t report_size = static_cast<uint8_t>(11 + data_len);
    const bool ok = tuh_hid_send_report(address, instance, 0, &out_report_, report_size);
    OGXM_LOG("SwitchProHost[%u]: send_subcommand addr=%u inst=%u subcmd=0x%02X seq=%u len=%u ok=%u\n",
             idx_, address, instance, sub_command, out_report_.sequence_counter, report_size, ok ? 1 : 0);
    return ok;
}

bool SwitchProHost::send_keepalive_rumble(uint8_t address, uint8_t instance, const char* reason)
{
    if (!rumble_capable_)
    {
        OGXM_LOG("SwitchProHost[%u]: keepalive skipped, rumble unavailable state=%s reason=%s\n",
                 idx_, init_state_name(init_state_), (reason != nullptr) ? reason : "n/a");
        return false;
    }

    std::memset(&out_report_, 0, sizeof(out_report_));
    out_report_.command = SwitchPro::REPORT::OUTPUT_RUMBLE_ONLY;
    out_report_.sequence_counter = get_output_sequence_counter();
    SwitchProRumble::fill_neutral(out_report_.rumble_l);
    SwitchProRumble::fill_neutral(out_report_.rumble_r);

    const bool ok = tuh_hid_send_report(address, instance, 0, &out_report_, 10);
    OGXM_LOG("SwitchProHost[%u]: keepalive send addr=%u inst=%u seq=%u budget=%u ok=%u reason=%s\n",
             idx_, address, instance, out_report_.sequence_counter, ready_keepalive_budget_,
             ok ? 1 : 0, (reason != nullptr) ? reason : "n/a");
    return ok;
}

bool SwitchProHost::send_control_hid_command(uint8_t address, uint8_t instance, uint8_t command)
{
    control_hid_command_.report_id = SwitchPro::REPORT::OUTPUT_HID;
    control_hid_command_.command = command;

    const bool ok = tuh_hid_set_report(address, instance, SwitchPro::REPORT::OUTPUT_HID,
                                       HID_REPORT_TYPE_OUTPUT, &control_hid_command_.command, 1);
    OGXM_LOG("SwitchProHost[%u]: send_control_hid_command addr=%u inst=%u cmd=0x%02X ok=%u\n",
             idx_, address, instance, command, ok ? 1 : 0);
    return ok;
}

bool SwitchProHost::send_control_subcommand(uint8_t address, uint8_t instance, uint8_t sub_command,
                                            const uint8_t* data, uint8_t data_len)
{
    std::memset(&control_out_report_, 0, sizeof(control_out_report_));

    control_out_report_.command = SwitchPro::REPORT::OUTPUT_SUBCMD;
    control_out_report_.sequence_counter = get_output_sequence_counter();
    SwitchProRumble::fill_neutral(control_out_report_.rumble_l);
    SwitchProRumble::fill_neutral(control_out_report_.rumble_r);
    control_out_report_.sub_command = sub_command;

    if (data != nullptr && data_len > 0)
    {
        if (data_len > sizeof(control_out_report_.sub_command_args))
        {
            data_len = sizeof(control_out_report_.sub_command_args);
        }
        std::memcpy(control_out_report_.sub_command_args, data, data_len);
    }

    const uint16_t payload_size = static_cast<uint16_t>(10 + data_len);
    const bool ok = tuh_hid_set_report(address, instance, SwitchPro::REPORT::OUTPUT_SUBCMD,
                                       HID_REPORT_TYPE_OUTPUT, &control_out_report_.sequence_counter,
                                       payload_size);
    OGXM_LOG("SwitchProHost[%u]: send_control_subcommand addr=%u inst=%u subcmd=0x%02X seq=%u len=%u ok=%u\n",
             idx_, address, instance, sub_command, control_out_report_.sequence_counter,
             payload_size, ok ? 1 : 0);
    return ok;
}

bool SwitchProHost::send_control_get_report(uint8_t address, uint8_t instance, uint8_t report_id,
                                            uint8_t report_type, uint16_t len)
{
    if (len > control_get_report_buffer_.size())
    {
        len = static_cast<uint16_t>(control_get_report_buffer_.size());
    }

    control_get_report_buffer_.fill(0);

    const bool ok = tuh_hid_get_report(address, instance, report_id, report_type,
                                       control_get_report_buffer_.data(), len);
    OGXM_LOG("SwitchProHost[%u]: send_control_get_report addr=%u inst=%u report=0x%02X type=%u len=%u ok=%u\n",
             idx_, address, instance, report_id, report_type, len, ok ? 1 : 0);
    return ok;
}

void SwitchProHost::init_switch_host(uint8_t address, uint8_t instance)
{
    const uint8_t full_report_mode[] = { SwitchPro::CMD::FULL_REPORT_MODE };
    const uint8_t enable_vibration[] = { 1 };
    const uint8_t enable_imu[] = { 1 };
    const uint8_t player_led[] = { static_cast<uint8_t>(idx_ + 1) };
    const uint8_t home_led[] = {
        static_cast<uint8_t>((0 << 4) | 0x0F),
        static_cast<uint8_t>((0x0F << 4) | 0x00),
        static_cast<uint8_t>((0x0F << 4) | 0x00),
    };

    switch (init_state_)
    {
        case InitState::HANDSHAKE:
            if (send_hid_command(address, instance, SwitchPro::CMD::HANDSHAKE))
            {
                set_init_state(InitState::DISABLE_TIMEOUT, "handshake queued");
            }
            break;

        case InitState::DISABLE_TIMEOUT:
            if (send_hid_command(address, instance, SwitchPro::CMD::DISABLE_TIMEOUT))
            {
                set_init_state(InitState::SET_FULL_REPORT, "disable-timeout queued");
            }
            break;

        case InitState::SET_FULL_REPORT:
            if (clone_full_report_ready_at_ms_ != 0)
            {
                const uint32_t now_ms = board_api::ms_since_boot();
                if (now_ms < clone_full_report_ready_at_ms_)
                {
                    if (!clone_full_report_delay_logged_)
                    {
                        OGXM_LOG("SwitchProHost[%u]: clone path delaying MODE 0x03 for settle window remaining=%u ms\n",
                                 idx_, clone_full_report_ready_at_ms_ - now_ms);
                        clone_full_report_delay_logged_ = true;
                    }
                    break;
                }
            }

            if (send_subcommand(address, instance, SwitchPro::CMD::MODE, full_report_mode, sizeof(full_report_mode)))
            {
                init_timeout_count_ = 0;
                clone_full_report_ready_at_ms_ = 0;
                clone_full_report_delay_logged_ = false;
                set_init_state(InitState::WAIT_FULL_REPORT, "full-report mode queued");
            }
            break;

        case InitState::ENABLE_VIBRATION:
            if (send_subcommand(address, instance, SwitchPro::CMD::ENABLE_VIBRATION, enable_vibration, sizeof(enable_vibration)))
            {
                init_timeout_count_ = 0;
                set_init_state(InitState::WAIT_VIBRATION, "enable-vibration queued");
            }
            break;

        case InitState::ENABLE_IMU:
            if (send_subcommand(address, instance, SwitchPro::CMD::GYRO, enable_imu, sizeof(enable_imu)))
            {
                init_timeout_count_ = 0;
                set_init_state(InitState::WAIT_IMU, "enable-imu queued");
            }
            break;

        case InitState::SET_LED:
            if (send_subcommand(address, instance, SwitchPro::CMD::LED, player_led, sizeof(player_led)))
            {
                init_timeout_count_ = 0;
                set_init_state(InitState::WAIT_LED, "set-player-led queued");
            }
            break;

        case InitState::SET_HOME_LED:
            if (send_subcommand(address, instance, SwitchPro::CMD::LED_HOME, home_led, sizeof(home_led)))
            {
                init_timeout_count_ = 0;
                set_init_state(InitState::WAIT_HOME_LED, "set-home-led queued");
            }
            break;

        case InitState::WAIT_FULL_REPORT:
            if (init_timeout_count_ > 0 && (init_timeout_count_ % INIT_RETRY_INTERVAL) == 0)
            {
                send_subcommand(address, instance, SwitchPro::CMD::MODE, full_report_mode, sizeof(full_report_mode));
            }
            break;

        case InitState::WAIT_VIBRATION:
            if (init_timeout_count_ > 0 && (init_timeout_count_ % INIT_RETRY_INTERVAL) == 0)
            {
                send_subcommand(address, instance, SwitchPro::CMD::ENABLE_VIBRATION, enable_vibration, sizeof(enable_vibration));
            }
            break;

        case InitState::WAIT_IMU:
            if (init_timeout_count_ > 0 && (init_timeout_count_ % INIT_RETRY_INTERVAL) == 0)
            {
                send_subcommand(address, instance, SwitchPro::CMD::GYRO, enable_imu, sizeof(enable_imu));
            }
            break;

        case InitState::WAIT_LED:
            if (init_timeout_count_ > 0 && (init_timeout_count_ % INIT_RETRY_INTERVAL) == 0)
            {
                send_subcommand(address, instance, SwitchPro::CMD::LED, player_led, sizeof(player_led));
            }
            break;

        case InitState::WAIT_HOME_LED:
            if (init_timeout_count_ > 0 && (init_timeout_count_ % INIT_RETRY_INTERVAL) == 0)
            {
                send_subcommand(address, instance, SwitchPro::CMD::LED_HOME, home_led, sizeof(home_led));
            }
            break;

        default:
            break;
    }
}

void SwitchProHost::advance_after_full_report()
{
    full_report_seen_ = true;
    init_timeout_count_ = 0;
    set_init_state(InitState::ENABLE_VIBRATION, "full report acknowledged");
}

void SwitchProHost::advance_after_vibration()
{
    rumble_capable_ = true;
    init_timeout_count_ = 0;
    set_init_state(InitState::ENABLE_IMU, "vibration acknowledged");
}

void SwitchProHost::advance_after_imu()
{
    imu_enabled_ = true;
    init_timeout_count_ = 0;
    set_init_state(InitState::SET_LED, "imu acknowledged");
}

void SwitchProHost::advance_after_led()
{
    init_timeout_count_ = 0;
    set_init_state(InitState::SET_HOME_LED, "player led acknowledged");
}

void SwitchProHost::advance_after_home_led()
{
    init_timeout_count_ = 0;
    ready_keepalive_budget_ = READY_KEEPALIVE_BURST;
    if (clone_init_path_active_ && !input_stream_seen_ && !clone_ready_mode_retry_attempted_)
    {
        clone_ready_mode_retry_pending_ = true;
        clone_ready_mode_retry_at_ms_ = board_api::ms_since_boot() + CLONE_READY_MODE_RETRY_DELAY_MS;
        OGXM_LOG("SwitchProHost[%u]: clone path scheduled post-ready MODE 0x03 retry delay=%u ms\n",
                 idx_, CLONE_READY_MODE_RETRY_DELAY_MS);
    }
    set_init_state(imu_enabled_ ? InitState::READY_FULL : InitState::READY_COMPAT_RUMBLE,
                   "home led acknowledged");
}

void SwitchProHost::observe_vendor_status_report(uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    if (report == nullptr || len == 0 || report[0] != 0x81)
    {
        return;
    }

    saw_vendor_status_report_ = true;

    if (!clone_init_path_active_ &&
        len >= sizeof(CLONE_VENDOR_STATUS_SIGNATURE) &&
        std::memcmp(report, CLONE_VENDOR_STATUS_SIGNATURE, sizeof(CLONE_VENDOR_STATUS_SIGNATURE)) == 0)
    {
        activate_parallel_clone_path("matched clone 0x81 vendor-status fingerprint");
    }

    if (init_state_ == InitState::SET_FULL_REPORT && clone_full_report_ready_at_ms_ == 0)
    {
        clone_full_report_ready_at_ms_ = board_api::ms_since_boot() + CLONE_FULL_REPORT_SETTLE_DELAY_MS;
        clone_full_report_delay_logged_ = false;
        OGXM_LOG("SwitchProHost[%u]: scheduling MODE 0x03 settle delay=%u ms after 0x81 vendor-status\n",
                 idx_, CLONE_FULL_REPORT_SETTLE_DELAY_MS);
    }

    if (vendor_status_report_logs_ >= MAX_VENDOR_STATUS_REPORT_LOGS)
    {
        return;
    }

    const uint8_t b0 = (len > 0) ? report[0] : 0;
    const uint8_t b1 = (len > 1) ? report[1] : 0;
    const uint8_t b2 = (len > 2) ? report[2] : 0;
    const uint8_t b3 = (len > 3) ? report[3] : 0;
    const uint8_t b4 = (len > 4) ? report[4] : 0;
    const uint8_t b5 = (len > 5) ? report[5] : 0;
    const uint8_t b6 = (len > 6) ? report[6] : 0;
    const uint8_t b7 = (len > 7) ? report[7] : 0;

    OGXM_LOG("SwitchProHost[%u]: vendor_status_0x81 addr=%u inst=%u len=%u state=%s bytes=%02X %02X %02X %02X %02X %02X %02X %02X\n",
             idx_, address, instance, len, init_state_name(init_state_),
             b0, b1, b2, b3, b4, b5, b6, b7);

    ++vendor_status_report_logs_;
    if (vendor_status_report_logs_ == MAX_VENDOR_STATUS_REPORT_LOGS)
    {
        OGXM_LOG("SwitchProHost[%u]: suppressing further 0x81 vendor-status logs\n", idx_);
    }
}

void SwitchProHost::mark_input_stream_seen(uint8_t report_id)
{
    if (input_stream_seen_)
    {
        return;
    }

    input_stream_seen_ = true;
    clone_ready_mode_retry_pending_ = false;
    clone_ready_mode_retry_at_ms_ = 0;
    ready_keepalive_budget_ = 0;
    control_fallback_active_ = false;
    get_report_probe_active_ = false;
    OGXM_LOG("SwitchProHost[%u]: input stream detected report=0x%02X, stopping fallback probes\n",
             idx_, report_id);
}

bool SwitchProHost::queue_control_fallback_step(uint8_t address, uint8_t instance)
{
    const uint8_t full_report_mode[] = { SwitchPro::CMD::FULL_REPORT_MODE };

    switch (control_fallback_state_)
    {
        case ControlFallbackState::HANDSHAKE:
            return send_control_hid_command(address, instance, SwitchPro::CMD::HANDSHAKE);
        case ControlFallbackState::DISABLE_TIMEOUT:
            return send_control_hid_command(address, instance, SwitchPro::CMD::DISABLE_TIMEOUT);
        case ControlFallbackState::SET_FULL_REPORT:
            return send_control_subcommand(address, instance, SwitchPro::CMD::MODE,
                                           full_report_mode, sizeof(full_report_mode));
        default:
            return false;
    }
}

bool SwitchProHost::queue_get_report_probe_step(uint8_t address, uint8_t instance)
{
    uint8_t report_id = 0;
    uint16_t report_len = 64;

    switch (get_report_probe_state_)
    {
        case GetReportProbeState::INPUT_0x30:
            report_id = SwitchPro::REPORT::INPUT_IMU_DATA;
            report_len = static_cast<uint16_t>(sizeof(SwitchPro::InReport));
            break;

        case GetReportProbeState::INPUT_0x81:
            report_id = 0x81;
            report_len = 64;
            break;

        case GetReportProbeState::INPUT_0x21:
            report_id = SwitchPro::REPORT::INPUT_SUBCMD_REPLY;
            report_len = 64;
            break;

        default:
            return false;
    }

    if (!send_control_get_report(address, instance, report_id, HID_REPORT_TYPE_INPUT, report_len))
    {
        return false;
    }

    get_report_probe_started_at_ms_ = board_api::ms_since_boot();
    return true;
}

void SwitchProHost::maybe_start_control_fallback(uint8_t address, uint8_t instance, const char* reason)
{
    if (input_stream_seen_ || control_fallback_attempted_ || control_fallback_active_ || !is_ready())
    {
        return;
    }

    if (clone_init_path_active_ && clone_ready_mode_retry_pending_)
    {
        OGXM_LOG("SwitchProHost[%u]: delaying control fallback until clone MODE 0x03 retry window expires\n",
                 idx_);
        return;
    }

    if (clone_init_path_active_ &&
        clone_ready_mode_retry_attempted_ &&
        board_api::ms_since_boot() < clone_ready_mode_retry_at_ms_)
    {
        OGXM_LOG("SwitchProHost[%u]: delaying control fallback while awaiting clone MODE 0x03 retry result\n",
                 idx_);
        return;
    }

    control_fallback_attempted_ = true;
    control_fallback_active_ = true;
    control_fallback_state_ = ControlFallbackState::HANDSHAKE;

    OGXM_LOG("SwitchProHost[%u]: starting control fallback via SET_REPORT state=%s reason=%s\n",
             idx_, control_fallback_state_name(control_fallback_state_),
             (reason != nullptr) ? reason : "n/a");

    if (!queue_control_fallback_step(address, instance))
    {
        control_fallback_active_ = false;
        control_fallback_state_ = ControlFallbackState::FAILED;
        OGXM_LOG("SwitchProHost[%u]: control fallback failed to queue initial step\n", idx_);
    }
}

void SwitchProHost::maybe_start_get_report_probe(uint8_t address, uint8_t instance, const char* reason)
{
    if (input_stream_seen_ || get_report_probe_attempted_ || get_report_probe_active_ ||
        control_fallback_active_ || !is_ready())
    {
        return;
    }

    const bool prioritize_clone_reports = clone_init_path_active_ && saw_vendor_status_report_;
    get_report_probe_attempted_ = true;
    get_report_probe_active_ = true;
    get_report_probe_state_ = prioritize_clone_reports ? GetReportProbeState::INPUT_0x81
                                                       : GetReportProbeState::INPUT_0x30;
    OGXM_LOG("SwitchProHost[%u]: starting get-report probe state=%s reason=%s\n",
             idx_, get_report_probe_state_name(get_report_probe_state_),
             (reason != nullptr) ? reason : "n/a");

    if (!queue_get_report_probe_step(address, instance))
    {
        get_report_probe_active_ = false;
        get_report_probe_started_at_ms_ = 0;
        get_report_probe_state_ = GetReportProbeState::FAILED;
        OGXM_LOG("SwitchProHost[%u]: get-report probe failed to queue initial step\n", idx_);
    }
}

void SwitchProHost::advance_control_fallback(uint8_t address, uint8_t instance, bool success,
                                             uint8_t report_id, uint8_t report_type, uint16_t len)
{
    (void)report_type;

    if (!control_fallback_active_)
    {
        return;
    }

    OGXM_LOG("SwitchProHost[%u]: control fallback complete state=%s report=0x%02X len=%u success=%u\n",
             idx_, control_fallback_state_name(control_fallback_state_), report_id, len, success ? 1 : 0);

    if (!success)
    {
        control_fallback_active_ = false;
        control_fallback_state_ = ControlFallbackState::FAILED;
        OGXM_LOG("SwitchProHost[%u]: control fallback aborted after failed SET_REPORT\n", idx_);
        return;
    }

    switch (control_fallback_state_)
    {
        case ControlFallbackState::HANDSHAKE:
            control_fallback_state_ = ControlFallbackState::DISABLE_TIMEOUT;
            break;
        case ControlFallbackState::DISABLE_TIMEOUT:
            control_fallback_state_ = ControlFallbackState::SET_FULL_REPORT;
            break;
        case ControlFallbackState::SET_FULL_REPORT:
            control_fallback_state_ = ControlFallbackState::DONE;
            control_fallback_active_ = false;
            ready_keepalive_budget_ = READY_KEEPALIVE_BURST;
            OGXM_LOG("SwitchProHost[%u]: control fallback finished after MODE set-report\n", idx_);
            if (saw_vendor_status_report_)
            {
                OGXM_LOG("SwitchProHost[%u]: starting automatic get-report probe after 0x81 vendor-status observation\n",
                         idx_);
                maybe_start_get_report_probe(address, instance, "post-control-fallback-after-0x81");
            }
            else
            {
                OGXM_LOG("SwitchProHost[%u]: starting automatic get-report probe without additional clone observations\n",
                         idx_);
                maybe_start_get_report_probe(address, instance, "post-control-fallback-no-vendor-status");
            }
            return;
        default:
            control_fallback_active_ = false;
            control_fallback_state_ = ControlFallbackState::FAILED;
            OGXM_LOG("SwitchProHost[%u]: control fallback entered invalid state\n", idx_);
            return;
    }

    if (!queue_control_fallback_step(address, instance))
    {
        control_fallback_active_ = false;
        control_fallback_state_ = ControlFallbackState::FAILED;
        OGXM_LOG("SwitchProHost[%u]: control fallback failed to queue next step=%s\n",
                 idx_, control_fallback_state_name(control_fallback_state_));
    }
}

void SwitchProHost::advance_get_report_probe(uint8_t address, uint8_t instance, bool success,
                                             uint8_t report_id, uint8_t report_type, uint16_t len)
{
    if (!get_report_probe_active_)
    {
        return;
    }

    const bool prioritize_clone_reports = clone_init_path_active_ && saw_vendor_status_report_;
    const uint8_t b0 = (len > 0) ? control_get_report_buffer_[0] : 0;
    const uint8_t b1 = (len > 1) ? control_get_report_buffer_[1] : 0;
    const uint8_t b2 = (len > 2) ? control_get_report_buffer_[2] : 0;
    const uint8_t b3 = (len > 3) ? control_get_report_buffer_[3] : 0;
    get_report_probe_started_at_ms_ = 0;

    OGXM_LOG("SwitchProHost[%u]: get-report complete state=%s report=0x%02X type=%u len=%u success=%u bytes=%02X %02X %02X %02X\n",
             idx_, get_report_probe_state_name(get_report_probe_state_), report_id, report_type, len,
             success ? 1 : 0, b0, b1, b2, b3);

    switch (get_report_probe_state_)
    {
        case GetReportProbeState::INPUT_0x30:
            get_report_probe_state_ = GetReportProbeState::INPUT_0x81;
            if (!queue_get_report_probe_step(address, instance))
            {
                get_report_probe_active_ = false;
                get_report_probe_state_ = GetReportProbeState::FAILED;
                OGXM_LOG("SwitchProHost[%u]: get-report probe failed to queue next step=%s\n",
                         idx_, get_report_probe_state_name(get_report_probe_state_));
            }
            return;

        case GetReportProbeState::INPUT_0x81:
            get_report_probe_state_ = GetReportProbeState::INPUT_0x21;
            if (!queue_get_report_probe_step(address, instance))
            {
                get_report_probe_active_ = false;
                get_report_probe_state_ = GetReportProbeState::FAILED;
                OGXM_LOG("SwitchProHost[%u]: get-report probe failed to queue next step=%s\n",
                         idx_, get_report_probe_state_name(get_report_probe_state_));
            }
            return;

        case GetReportProbeState::INPUT_0x21:
            if (prioritize_clone_reports)
            {
                get_report_probe_state_ = GetReportProbeState::DONE;
                get_report_probe_active_ = false;
                OGXM_LOG("SwitchProHost[%u]: clone-prioritized get-report probe finished without interrupt input stream\n",
                         idx_);
                return;
            }

            get_report_probe_state_ = GetReportProbeState::DONE;
            get_report_probe_active_ = false;
            OGXM_LOG("SwitchProHost[%u]: get-report probe finished without interrupt input stream\n", idx_);
            return;

        default:
            get_report_probe_active_ = false;
            get_report_probe_state_ = GetReportProbeState::FAILED;
            OGXM_LOG("SwitchProHost[%u]: get-report probe entered invalid state\n", idx_);
            return;
    }
}

void SwitchProHost::handle_timeout()
{
    switch (init_state_)
    {
        case InitState::WAIT_FULL_REPORT:
            if (full_report_seen_)
            {
                init_timeout_count_ = 0;
                set_init_state(InitState::ENABLE_VIBRATION, "full report already seen before timeout");
            }
            else
            {
                if (using_compat_fallback_)
                {
                    OGXM_LOG("SwitchProHost: Full report setup timed out after compatibility reports, trying vibration enable in compatibility mode.\n");
                    init_timeout_count_ = 0;
                    set_init_state(InitState::ENABLE_VIBRATION, "full report timeout after compat report");
                }
                else
                {
                    OGXM_LOG("SwitchProHost: Full report setup timed out, falling back to input-only compatibility.\n");
                    using_compat_fallback_ = true;
                    rumble_capable_ = false;
                    set_init_state(InitState::READY_COMPAT_INPUT, "full report timeout");
                }
            }
            break;

        case InitState::WAIT_VIBRATION:
            OGXM_LOG("SwitchProHost: Vibration enable timed out, continuing without rumble.\n");
            rumble_capable_ = false;
            init_timeout_count_ = 0;
            set_init_state(InitState::ENABLE_IMU, "vibration timeout");
            break;

        case InitState::WAIT_IMU:
            OGXM_LOG("SwitchProHost: IMU setup timed out, continuing without IMU.\n");
            init_timeout_count_ = 0;
            set_init_state(InitState::SET_LED, "imu timeout");
            break;

        case InitState::WAIT_LED:
            OGXM_LOG("SwitchProHost: Player LED setup timed out, continuing.\n");
            init_timeout_count_ = 0;
            set_init_state(InitState::SET_HOME_LED, "player led timeout");
            break;

        case InitState::WAIT_HOME_LED:
            OGXM_LOG("SwitchProHost: Home LED setup timed out, finalizing rumble-compatible mode.\n");
            init_timeout_count_ = 0;
            set_init_state(rumble_capable_ ? InitState::READY_COMPAT_RUMBLE : InitState::READY_COMPAT_INPUT,
                           "home led timeout");
            break;

        default:
            break;
    }
}

void SwitchProHost::update_init_state(uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    (void)address;
    (void)instance;

    if (report == nullptr || len == 0)
    {
        return;
    }

    if (len == sizeof(SwitchWired::InReport) || len == 8 || report[0] == SwitchPro::REPORT::INPUT_BUTTON_EVENT)
    {
        mark_input_stream_seen(report[0]);
    }
    else if (report[0] == SwitchPro::REPORT::INPUT_IMU_DATA)
    {
        mark_input_stream_seen(report[0]);
    }

    if (is_ready())
    {
        return;
    }

    if (debug_input_report_logs_ < MAX_DEBUG_INPUT_REPORT_LOGS)
    {
        const uint8_t b0 = (len > 0) ? report[0] : 0;
        const uint8_t b1 = (len > 1) ? report[1] : 0;
        const uint8_t b2 = (len > 2) ? report[2] : 0;
        const uint8_t b3 = (len > 3) ? report[3] : 0;
        OGXM_LOG("SwitchProHost[%u]: recv addr=%u inst=%u len=%u id=0x%02X state=%s bytes=%02X %02X %02X %02X\n",
                 idx_, address, instance, len, b0, init_state_name(init_state_), b0, b1, b2, b3);
        ++debug_input_report_logs_;
        if (debug_input_report_logs_ == MAX_DEBUG_INPUT_REPORT_LOGS)
        {
            OGXM_LOG("SwitchProHost[%u]: suppressing further input report debug logs\n", idx_);
        }
    }

    if (len == sizeof(SwitchWired::InReport) || len == 8 || report[0] == SwitchPro::REPORT::INPUT_BUTTON_EVENT)
    {
        if (!using_compat_fallback_)
        {
            OGXM_LOG("SwitchProHost[%u]: compatibility report detected len=%u id=0x%02X\n",
                     idx_, len, report[0]);
        }
        using_compat_fallback_ = true;
    }

    if (report[0] == SwitchPro::REPORT::INPUT_IMU_DATA)
    {
        full_report_seen_ = true;
        if (init_state_ == InitState::WAIT_FULL_REPORT)
        {
            advance_after_full_report();
        }
        return;
    }

    if (report[0] == 0x81)
    {
        observe_vendor_status_report(address, instance, report, len);
        return;
    }

    if (report[0] == SwitchPro::REPORT::INPUT_SUBCMD_REPLY && len >= sizeof(SwitchPro::SubcommandReply))
    {
        const auto* reply = reinterpret_cast<const SwitchPro::SubcommandReply*>(report);
        const bool success = (reply->ack & SwitchPro::CMD::ACK_SUCCESS) != 0;

        OGXM_LOG("SwitchProHost[%u]: subcmd_reply ack=0x%02X subcmd=0x%02X success=%u state=%s\n",
                 idx_, reply->ack, reply->sub_command, success ? 1 : 0, init_state_name(init_state_));

        switch (reply->sub_command)
        {
            case SwitchPro::CMD::MODE:
                if (init_state_ == InitState::WAIT_FULL_REPORT)
                {
                    if (success)
                    {
                        advance_after_full_report();
                    }
                    else
                    {
                        handle_timeout();
                    }
                }
                return;

            case SwitchPro::CMD::ENABLE_VIBRATION:
                if (init_state_ == InitState::WAIT_VIBRATION)
                {
                    if (success)
                    {
                        advance_after_vibration();
                    }
                    else
                    {
                        handle_timeout();
                    }
                }
                return;

            case SwitchPro::CMD::GYRO:
                if (init_state_ == InitState::WAIT_IMU)
                {
                    if (success)
                    {
                        advance_after_imu();
                    }
                    else
                    {
                        handle_timeout();
                    }
                }
                return;

            case SwitchPro::CMD::LED:
                if (init_state_ == InitState::WAIT_LED)
                {
                    if (success)
                    {
                        advance_after_led();
                    }
                    else
                    {
                        handle_timeout();
                    }
                }
                return;

            case SwitchPro::CMD::LED_HOME:
                if (init_state_ == InitState::WAIT_HOME_LED)
                {
                    if (success)
                    {
                        advance_after_home_led();
                    }
                    else
                    {
                        handle_timeout();
                    }
                }
                return;

            default:
                return;
        }
    }

    switch (init_state_)
    {
        case InitState::WAIT_FULL_REPORT:
        case InitState::WAIT_VIBRATION:
        case InitState::WAIT_IMU:
        case InitState::WAIT_LED:
        case InitState::WAIT_HOME_LED:
        {
            ++init_timeout_count_;
            const uint32_t timeout_limit =
                (init_state_ == InitState::WAIT_FULL_REPORT) ? INIT_TIMEOUT_FULL_REPORT : INIT_TIMEOUT_OPTIONAL;

            if (init_timeout_count_ >= timeout_limit)
            {
                handle_timeout();
            }
            break;
        }
        default:
            break;
    }
}

bool SwitchProHost::parse_switch_wired_report(Gamepad& gamepad, const uint8_t* report, uint16_t len)
{
    if (len != sizeof(SwitchWired::InReport) && len != 8)
    {
        return false;
    }

    const auto* wired_report = reinterpret_cast<const SwitchWired::InReport*>(report);
    Gamepad::PadIn gp_in;

    switch (wired_report->dpad)
    {
        case SwitchWired::DPad::UP:         gp_in.dpad |= gamepad.MAP_DPAD_UP; break;
        case SwitchWired::DPad::DOWN:       gp_in.dpad |= gamepad.MAP_DPAD_DOWN; break;
        case SwitchWired::DPad::LEFT:       gp_in.dpad |= gamepad.MAP_DPAD_LEFT; break;
        case SwitchWired::DPad::RIGHT:      gp_in.dpad |= gamepad.MAP_DPAD_RIGHT; break;
        case SwitchWired::DPad::UP_RIGHT:   gp_in.dpad |= gamepad.MAP_DPAD_UP_RIGHT; break;
        case SwitchWired::DPad::DOWN_RIGHT: gp_in.dpad |= gamepad.MAP_DPAD_DOWN_RIGHT; break;
        case SwitchWired::DPad::DOWN_LEFT:  gp_in.dpad |= gamepad.MAP_DPAD_DOWN_LEFT; break;
        case SwitchWired::DPad::UP_LEFT:    gp_in.dpad |= gamepad.MAP_DPAD_UP_LEFT; break;
        default: break;
    }

    if (wired_report->buttons & SwitchWired::Buttons::Y)       gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (wired_report->buttons & SwitchWired::Buttons::B)       gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (wired_report->buttons & SwitchWired::Buttons::A)       gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (wired_report->buttons & SwitchWired::Buttons::X)       gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (wired_report->buttons & SwitchWired::Buttons::L)       gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (wired_report->buttons & SwitchWired::Buttons::R)       gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (wired_report->buttons & SwitchWired::Buttons::MINUS)   gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (wired_report->buttons & SwitchWired::Buttons::PLUS)    gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (wired_report->buttons & SwitchWired::Buttons::HOME)    gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (wired_report->buttons & SwitchWired::Buttons::CAPTURE) gp_in.buttons |= gamepad.MAP_BUTTON_MISC;
    if (wired_report->buttons & SwitchWired::Buttons::L3)      gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (wired_report->buttons & SwitchWired::Buttons::R3)      gp_in.buttons |= gamepad.MAP_BUTTON_R3;

    gp_in.trigger_l = (wired_report->buttons & SwitchWired::Buttons::ZL) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
    gp_in.trigger_r = (wired_report->buttons & SwitchWired::Buttons::ZR) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(wired_report->joystick_lx, wired_report->joystick_ly);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(wired_report->joystick_rx, wired_report->joystick_ry);

    gamepad.set_pad_in(gp_in);
    return true;
}

bool SwitchProHost::parse_switch_button_report(Gamepad& gamepad, const uint8_t* report, uint16_t len)
{
    if (len < 10 || report[0] != SwitchPro::REPORT::INPUT_BUTTON_EVENT)
    {
        return false;
    }

    Gamepad::PadIn gp_in;

    if (report[1] & 0x01) gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (report[1] & 0x02) gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (report[1] & 0x04) gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (report[1] & 0x08) gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (report[1] & 0x40) gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (report[1] & 0x80) gp_in.trigger_r = Range::MAX<uint8_t>;

    if (report[2] & 0x01) gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (report[2] & 0x02) gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (report[2] & 0x04) gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (report[2] & 0x08) gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (report[2] & 0x10) gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (report[2] & 0x20) gp_in.buttons |= gamepad.MAP_BUTTON_MISC;
    if (report[2] & 0x40) gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (report[2] & 0x80) gp_in.trigger_l = Range::MAX<uint8_t>;

    switch (report[3])
    {
        case 0: gp_in.dpad |= gamepad.MAP_DPAD_UP; break;
        case 1: gp_in.dpad |= gamepad.MAP_DPAD_UP_RIGHT; break;
        case 2: gp_in.dpad |= gamepad.MAP_DPAD_RIGHT; break;
        case 3: gp_in.dpad |= gamepad.MAP_DPAD_DOWN_RIGHT; break;
        case 4: gp_in.dpad |= gamepad.MAP_DPAD_DOWN; break;
        case 5: gp_in.dpad |= gamepad.MAP_DPAD_DOWN_LEFT; break;
        case 6: gp_in.dpad |= gamepad.MAP_DPAD_LEFT; break;
        case 7: gp_in.dpad |= gamepad.MAP_DPAD_UP_LEFT; break;
        default: break;
    }

    const uint16_t joy_lx = report[4] | ((report[5] & 0x0F) << 8);
    const uint16_t joy_ly = (report[5] >> 4) | (report[6] << 4);
    const uint16_t joy_rx = report[7] | ((report[8] & 0x0F) << 8);
    const uint16_t joy_ry = (report[8] >> 4) | (report[9] << 4);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(normalize_axis(joy_lx), normalize_axis(joy_ly), true);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(normalize_axis(joy_rx), normalize_axis(joy_ry), true);

    gamepad.set_pad_in(gp_in);
    return true;
}

bool SwitchProHost::parse_switch_full_report(Gamepad& gamepad, const uint8_t* report, uint16_t len)
{
    if (len < sizeof(SwitchPro::InReport) || report[0] != SwitchPro::REPORT::INPUT_IMU_DATA)
    {
        return false;
    }

    const auto* in_report = reinterpret_cast<const SwitchPro::InReport*>(report);
    if (std::memcmp(&prev_in_report_.buttons, in_report->buttons, 9) == 0)
    {
        return true;
    }

    Gamepad::PadIn gp_in;

    if (in_report->buttons[0] & SwitchPro::Buttons0::Y)  gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report->buttons[0] & SwitchPro::Buttons0::B)  gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->buttons[0] & SwitchPro::Buttons0::A)  gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->buttons[0] & SwitchPro::Buttons0::X)  gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (in_report->buttons[2] & SwitchPro::Buttons2::L)  gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->buttons[0] & SwitchPro::Buttons0::R)  gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report->buttons[1] & SwitchPro::Buttons1::L3) gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (in_report->buttons[1] & SwitchPro::Buttons1::R3) gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (in_report->buttons[1] & SwitchPro::Buttons1::MINUS)   gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report->buttons[1] & SwitchPro::Buttons1::PLUS)    gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (in_report->buttons[1] & SwitchPro::Buttons1::HOME)    gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (in_report->buttons[1] & SwitchPro::Buttons1::CAPTURE) gp_in.buttons |= gamepad.MAP_BUTTON_MISC;

    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_UP)    gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_DOWN)  gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_LEFT)  gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_RIGHT) gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;

    gp_in.trigger_l = (in_report->buttons[2] & SwitchPro::Buttons2::ZL) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
    gp_in.trigger_r = (in_report->buttons[0] & SwitchPro::Buttons0::ZR) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;

    const uint16_t joy_lx = in_report->joysticks[0] | ((in_report->joysticks[1] & 0x0F) << 8);
    const uint16_t joy_ly = (in_report->joysticks[1] >> 4) | (in_report->joysticks[2] << 4);
    const uint16_t joy_rx = in_report->joysticks[3] | ((in_report->joysticks[4] & 0x0F) << 8);
    const uint16_t joy_ry = (in_report->joysticks[4] >> 4) | (in_report->joysticks[5] << 4);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) =
        gamepad.scale_joystick_l(normalize_axis(joy_lx), normalize_axis(joy_ly), true);

    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) =
        gamepad.scale_joystick_r(normalize_axis(joy_rx), normalize_axis(joy_ry), true);

    gamepad.set_pad_in(gp_in);
    std::memcpy(&prev_in_report_, in_report, sizeof(SwitchPro::InReport));
    return true;
}

void SwitchProHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    update_init_state(address, instance, report, len);

    bool parsed = parse_switch_wired_report(gamepad, report, len);
    if (!parsed)
    {
        parsed = parse_switch_button_report(gamepad, report, len);
    }
    if (!parsed)
    {
        parsed = parse_switch_full_report(gamepad, report, len);
    }

    if (!parsed)
    {
        const uint8_t report_id = (report != nullptr && len > 0) ? report[0] : 0;
        OGXM_LOG("SwitchProHost[%u]: unparsed report len=%u id=0x%02X state=%s\n",
                 idx_, len, report_id, init_state_name(init_state_));
    }

    if (!is_ready())
    {
        init_switch_host(address, instance);
    }
    else if (!input_stream_seen_ &&
             !control_fallback_attempted_ &&
             report != nullptr &&
             len > 0 &&
             report[0] == SwitchPro::REPORT::INPUT_SUBCMD_REPLY)
    {
        maybe_start_control_fallback(address, instance, "ready-subcmd-reply-no-input");
    }

    tuh_hid_receive_report(address, instance);
}

bool SwitchProHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    if (!is_ready())
    {
        OGXM_LOG("SwitchProHost[%u]: send_feedback while not ready, state=%s\n",
                 idx_, init_state_name(init_state_));
        init_switch_host(address, instance);
        return false;
    }

    if (!input_stream_seen_)
    {
        if (clone_init_path_active_)
        {
            const uint32_t now_ms = board_api::ms_since_boot();
            if (clone_ready_mode_retry_pending_)
            {
                if (now_ms >= clone_ready_mode_retry_at_ms_)
                {
                    const uint8_t full_report_mode[] = { SwitchPro::CMD::FULL_REPORT_MODE };
                    clone_ready_mode_retry_pending_ = false;
                    clone_ready_mode_retry_attempted_ = true;
                    clone_ready_mode_retry_at_ms_ = now_ms + CLONE_READY_MODE_RETRY_DELAY_MS;
                    OGXM_LOG("SwitchProHost[%u]: clone path sending post-ready MODE 0x03 retry\n", idx_);
                    send_subcommand(address, instance, SwitchPro::CMD::MODE,
                                    full_report_mode, sizeof(full_report_mode));
                    return false;
                }

                OGXM_LOG("SwitchProHost[%u]: clone path waiting for post-ready MODE 0x03 retry window\n", idx_);
                return false;
            }

            if (clone_ready_mode_retry_attempted_ &&
                !control_fallback_attempted_ &&
                now_ms >= clone_ready_mode_retry_at_ms_)
            {
                maybe_start_control_fallback(address, instance, "clone-post-ready-mode-retry-no-input");
                return false;
            }
        }

        if (get_report_probe_active_ &&
            get_report_probe_started_at_ms_ != 0 &&
            (board_api::ms_since_boot() - get_report_probe_started_at_ms_) >= GET_REPORT_PROBE_TIMEOUT_MS)
        {
            ++get_report_probe_timeout_count_;
            OGXM_LOG("SwitchProHost[%u]: get-report probe timeout state=%s count=%u\n",
                     idx_, get_report_probe_state_name(get_report_probe_state_), get_report_probe_timeout_count_);
            get_report_probe_started_at_ms_ = 0;

            if (get_report_probe_state_ == GetReportProbeState::INPUT_0x30)
            {
                get_report_probe_state_ = GetReportProbeState::INPUT_0x81;
                if (queue_get_report_probe_step(address, instance))
                {
                    return false;
                }
            }
            else if (clone_init_path_active_ &&
                     saw_vendor_status_report_ &&
                     get_report_probe_state_ == GetReportProbeState::INPUT_0x81)
            {
                get_report_probe_state_ = GetReportProbeState::INPUT_0x21;
                if (queue_get_report_probe_step(address, instance))
                {
                    return false;
                }
            }

            get_report_probe_active_ = false;
            get_report_probe_state_ = GetReportProbeState::FAILED;
            ready_keepalive_budget_ = READY_KEEPALIVE_BURST;

            if (get_report_probe_timeout_count_ < MAX_GET_REPORT_PROBE_TIMEOUTS)
            {
                get_report_probe_attempted_ = false;
                OGXM_LOG("SwitchProHost[%u]: get-report probe timed out, allowing retry\n", idx_);
            }
            else
            {
                OGXM_LOG("SwitchProHost[%u]: get-report probe timed out, retries exhausted\n", idx_);
            }
            return false;
        }

        if (ready_keepalive_budget_ > 0 &&
            !control_fallback_active_ &&
            !get_report_probe_active_)
        {
            --ready_keepalive_budget_;
            send_keepalive_rumble(address, instance, "awaiting-input-stream");
            return false;
        }

        if (control_fallback_attempted_ &&
            !get_report_probe_attempted_ &&
            !get_report_probe_active_)
        {
            maybe_start_get_report_probe(address, instance, "send-feedback-no-input-after-fallback");
            return false;
        }

        const Gamepad::PadOut dropped_out = gamepad.get_pad_out();
        OGXM_LOG("SwitchProHost[%u]: send_feedback suppressed before input stream rumble_l=%u rumble_r=%u\n",
                 idx_, dropped_out.rumble_l, dropped_out.rumble_r);
        return false;
    }

    if (!rumble_capable_)
    {
        OGXM_LOG("SwitchProHost[%u]: send_feedback ignored, rumble not available in state=%s\n",
                 idx_, init_state_name(init_state_));
        return false;
    }

    std::memset(&out_report_, 0, sizeof(out_report_));
    out_report_.command = SwitchPro::REPORT::OUTPUT_RUMBLE_ONLY;
    out_report_.sequence_counter = get_output_sequence_counter();

    const Gamepad::PadOut gp_out = gamepad.get_pad_out();
    SwitchProRumble::encode(out_report_.rumble_l, gp_out.rumble_l);
    SwitchProRumble::encode(out_report_.rumble_r, gp_out.rumble_r);

    const bool ok = tuh_hid_send_report(address, instance, 0, &out_report_, 10);
    OGXM_LOG("SwitchProHost[%u]: send_feedback rumble seq=%u ok=%u\n",
             idx_, out_report_.sequence_counter, ok ? 1 : 0);
    return ok;
}

void SwitchProHost::set_report_complete_cb(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                           uint8_t report_id, uint8_t report_type, uint16_t len)
{
    (void)gamepad;
    advance_control_fallback(address, instance, len > 0, report_id, report_type, len);
}

void SwitchProHost::get_report_complete_cb(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                           uint8_t report_id, uint8_t report_type, uint16_t len)
{
    (void)gamepad;
    advance_get_report_probe(address, instance, len > 0, report_id, report_type, len);
}

void SwitchProHost::disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)gamepad;
    (void)address;
    (void)instance;

    if (SwitchProCloneRecovery::clone_profile_detected())
    {
        SwitchProCloneRecovery::arm_next_attach_recovery();
        OGXM_LOG("SwitchProHost[%u]: clone recovery armed for next attach after disconnect\n", idx_);
    }
}

void SwitchProHost::report_sent_cb(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    (void)gamepad;

    const uint8_t report_id = (report != nullptr && len > 0) ? report[0] : 0;
    OGXM_LOG("SwitchProHost[%u]: report_sent_cb addr=%u inst=%u len=%u id=0x%02X state=%s\n",
             idx_, address, instance, len, report_id, init_state_name(init_state_));

    // Some Switch-compatible pads stay silent until the initial USB handshake
    // sequence has been fully transmitted. Advance the early init stages on
    // successful OUT transfers instead of waiting for an IN report first.
    if (!is_ready())
    {
        init_switch_host(address, instance);
        return;
    }

    maybe_start_control_fallback(address, instance, "interrupt-out-complete-without-input");
}
