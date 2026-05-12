#include <cstring>

#include "class/hid/hid_device.h"
#include <pico/time.h>

#include "Board/ogxm_log.h"
#include "USBDevice/DeviceDriver/Switch/Switch.h"

#ifndef OGXM_BT_RATE_DEBUG
#define OGXM_BT_RATE_DEBUG 0
#endif

#ifndef OGXM_SWITCH_SEND_ON_CHANGE_ONLY
#define OGXM_SWITCH_SEND_ON_CHANGE_ONLY 0
#endif

namespace {

bool pending_report[MAX_GAMEPADS]{};

#if OGXM_BT_RATE_DEBUG
struct UsbRateStats {
    uint32_t window_start_ms{0};
    uint32_t process_calls{0};
    uint32_t new_pad_in{0};
    uint32_t pending{0};
    uint32_t ready{0};
    uint32_t not_ready{0};
    uint32_t sent{0};
    uint32_t send_failed{0};
    uint32_t skipped_unchanged{0};
    uint32_t last_new_ms{0};
    uint32_t new_gap_min_ms{0};
    uint32_t new_gap_max_ms{0};
    uint32_t new_gap_lt5{0};
    uint32_t new_gap_lt10{0};
    uint32_t new_gap_10_20{0};
    uint32_t new_gap_20_60{0};
    uint32_t new_gap_60_75{0};
    uint32_t new_gap_gt75{0};
    uint32_t last_sent_ms{0};
    uint32_t sent_gap_min_ms{0};
    uint32_t sent_gap_max_ms{0};
    uint32_t sent_gap_lt5{0};
    uint32_t sent_gap_lt10{0};
    uint32_t sent_gap_10_20{0};
    uint32_t sent_gap_20_60{0};
    uint32_t sent_gap_60_75{0};
    uint32_t sent_gap_gt75{0};
};

UsbRateStats usb_rate_stats[MAX_GAMEPADS];

void log_usb_rate_stats(uint8_t idx)
{
    UsbRateStats& stats = usb_rate_stats[idx];
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    if (stats.window_start_ms == 0)
    {
        stats.window_start_ms = now_ms;
        return;
    }

    uint32_t elapsed_ms = now_ms - stats.window_start_ms;
    if (elapsed_ms < 1000)
    {
        return;
    }

    OGXM_LOG("[USB_RATE] switch idx=%u process=%lu new=%lu pending=%lu ready=%lu not_ready=%lu sent=%lu fail=%lu skip=%lu elapsed=%lu\n",
             idx,
             stats.process_calls,
             stats.new_pad_in,
             stats.pending,
             stats.ready,
             stats.not_ready,
             stats.sent,
             stats.send_failed,
             stats.skipped_unchanged,
             elapsed_ms);

    OGXM_LOG("[USB_GAP] switch idx=%u new_min=%lu new_max=%lu new_lt5=%lu new_lt10=%lu new_10_20=%lu new_20_60=%lu new_60_75=%lu new_gt75=%lu sent_min=%lu sent_max=%lu sent_lt5=%lu sent_lt10=%lu sent_10_20=%lu sent_20_60=%lu sent_60_75=%lu sent_gt75=%lu elapsed=%lu\n",
             idx,
             stats.new_gap_min_ms,
             stats.new_gap_max_ms,
             stats.new_gap_lt5,
             stats.new_gap_lt10,
             stats.new_gap_10_20,
             stats.new_gap_20_60,
             stats.new_gap_60_75,
             stats.new_gap_gt75,
             stats.sent_gap_min_ms,
             stats.sent_gap_max_ms,
             stats.sent_gap_lt5,
             stats.sent_gap_lt10,
             stats.sent_gap_10_20,
             stats.sent_gap_20_60,
             stats.sent_gap_60_75,
             stats.sent_gap_gt75,
             elapsed_ms);

    stats = UsbRateStats{};
    stats.window_start_ms = now_ms;
}

void record_gap(uint32_t& last_ms,
                uint32_t& min_gap_ms,
                uint32_t& max_gap_ms,
                uint32_t& gap_lt5,
                uint32_t& gap_lt10,
                uint32_t& gap_10_20,
                uint32_t& gap_20_60,
                uint32_t& gap_60_75,
                uint32_t& gap_gt75)
{
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if (last_ms != 0)
    {
        uint32_t gap_ms = now_ms - last_ms;
        if (min_gap_ms == 0 || gap_ms < min_gap_ms)
        {
            min_gap_ms = gap_ms;
        }
        if (gap_ms > max_gap_ms)
        {
            max_gap_ms = gap_ms;
        }

        if (gap_ms < 5)
            gap_lt5++;
        else if (gap_ms < 10)
            gap_lt10++;
        else if (gap_ms < 20)
            gap_10_20++;
        else if (gap_ms < 60)
            gap_20_60++;
        else if (gap_ms <= 75)
            gap_60_75++;
        else
            gap_gt75++;
    }

    last_ms = now_ms;
}
#endif

} // namespace

void SwitchDevice::initialize() 
{
	class_driver_ = 
    {
		.name = TUD_DRV_NAME("SWITCH"),
		.init = hidd_init,
        .deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};

    in_report_.fill(SwitchWired::InReport());
    for (bool& pending : pending_report)
    {
        pending = false;
    }
}

void SwitchDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    SwitchWired::InReport& in_report = in_report_[idx];
    bool has_new_pad_in = gamepad.new_pad_in();

#if OGXM_BT_RATE_DEBUG
    if (idx < MAX_GAMEPADS)
    {
        UsbRateStats& stats = usb_rate_stats[idx];
        stats.process_calls++;
        log_usb_rate_stats(idx);
    }
#endif

    if (has_new_pad_in)
    {
#if OGXM_BT_RATE_DEBUG
        if (idx < MAX_GAMEPADS)
        {
            usb_rate_stats[idx].new_pad_in++;
            record_gap(usb_rate_stats[idx].last_new_ms,
                       usb_rate_stats[idx].new_gap_min_ms,
                       usb_rate_stats[idx].new_gap_max_ms,
                       usb_rate_stats[idx].new_gap_lt5,
                       usb_rate_stats[idx].new_gap_lt10,
                       usb_rate_stats[idx].new_gap_10_20,
                       usb_rate_stats[idx].new_gap_20_60,
                       usb_rate_stats[idx].new_gap_60_75,
                       usb_rate_stats[idx].new_gap_gt75);
        }
#endif
        Gamepad::PadIn gp_in = gamepad.get_pad_in();
        pending_report[idx] = true;
    
        switch (gp_in.dpad)
        {
            case Gamepad::DPAD_UP:
                in_report.dpad = SwitchWired::DPad::UP;
                break;
            case Gamepad::DPAD_DOWN:
                in_report.dpad = SwitchWired::DPad::DOWN;
                break;
            case Gamepad::DPAD_LEFT:
                in_report.dpad = SwitchWired::DPad::LEFT;
                break;
            case Gamepad::DPAD_RIGHT:
                in_report.dpad = SwitchWired::DPad::RIGHT;
                break;
            case Gamepad::DPAD_UP_LEFT:
                in_report.dpad = SwitchWired::DPad::UP_LEFT;
                break;
            case Gamepad::DPAD_UP_RIGHT:
                in_report.dpad = SwitchWired::DPad::UP_RIGHT;
                break;
            case Gamepad::DPAD_DOWN_LEFT:
                in_report.dpad = SwitchWired::DPad::DOWN_LEFT;
                break;
            case Gamepad::DPAD_DOWN_RIGHT:
                in_report.dpad = SwitchWired::DPad::DOWN_RIGHT;
                break;
            default:
                in_report.dpad = SwitchWired::DPad::CENTER;
                break;
        }

        in_report.buttons = 0;

        if (gp_in.buttons & Gamepad::BUTTON_X)        in_report.buttons |= SwitchWired::Buttons::Y;
        if (gp_in.buttons & Gamepad::BUTTON_A)        in_report.buttons |= SwitchWired::Buttons::B;
        if (gp_in.buttons & Gamepad::BUTTON_Y)        in_report.buttons |= SwitchWired::Buttons::X;
        if (gp_in.buttons & Gamepad::BUTTON_B)        in_report.buttons |= SwitchWired::Buttons::A;
        if (gp_in.buttons & Gamepad::BUTTON_LB)       in_report.buttons |= SwitchWired::Buttons::L;
        if (gp_in.buttons & Gamepad::BUTTON_RB)       in_report.buttons |= SwitchWired::Buttons::R;
        if (gp_in.buttons & Gamepad::BUTTON_BACK)     in_report.buttons |= SwitchWired::Buttons::MINUS;
        if (gp_in.buttons & Gamepad::BUTTON_START)    in_report.buttons |= SwitchWired::Buttons::PLUS;
        if (gp_in.buttons & Gamepad::BUTTON_L3)       in_report.buttons |= SwitchWired::Buttons::L3;
        if (gp_in.buttons & Gamepad::BUTTON_R3)       in_report.buttons |= SwitchWired::Buttons::R3;
        if (gp_in.buttons & Gamepad::BUTTON_SYS)      in_report.buttons |= SwitchWired::Buttons::HOME;
        if (gp_in.buttons & Gamepad::BUTTON_MISC)     in_report.buttons |= SwitchWired::Buttons::CAPTURE;

        if (gp_in.trigger_l) in_report.buttons |= SwitchWired::Buttons::ZL;
        if (gp_in.trigger_r) in_report.buttons |= SwitchWired::Buttons::ZR;

        in_report.joystick_lx = Scale::int16_to_uint8(gp_in.joystick_lx);
        in_report.joystick_ly = Scale::int16_to_uint8(gp_in.joystick_ly);
        in_report.joystick_rx = Scale::int16_to_uint8(gp_in.joystick_rx);
        in_report.joystick_ry = Scale::int16_to_uint8(gp_in.joystick_ry);
    }

#if OGXM_SWITCH_SEND_ON_CHANGE_ONLY
    if (!pending_report[idx])
    {
#if OGXM_BT_RATE_DEBUG
        if (idx < MAX_GAMEPADS)
        {
            usb_rate_stats[idx].skipped_unchanged++;
        }
#endif
        return;
    }
#else
    pending_report[idx] = true;
#endif

#if OGXM_BT_RATE_DEBUG
    if (idx < MAX_GAMEPADS && pending_report[idx])
    {
        usb_rate_stats[idx].pending++;
    }
#endif

	if (tud_suspended())
    {
		tud_remote_wakeup();
    }
	if (tud_hid_n_ready(idx))
    {
#if OGXM_BT_RATE_DEBUG
        if (idx < MAX_GAMEPADS)
        {
            usb_rate_stats[idx].ready++;
        }
#endif
        bool sent = tud_hid_n_report(idx, 0, reinterpret_cast<uint8_t*>(&in_report), sizeof(SwitchWired::InReport));
        if (sent)
        {
            pending_report[idx] = false;
#if OGXM_BT_RATE_DEBUG
            if (idx < MAX_GAMEPADS)
            {
                usb_rate_stats[idx].sent++;
                record_gap(usb_rate_stats[idx].last_sent_ms,
                           usb_rate_stats[idx].sent_gap_min_ms,
                           usb_rate_stats[idx].sent_gap_max_ms,
                           usb_rate_stats[idx].sent_gap_lt5,
                           usb_rate_stats[idx].sent_gap_lt10,
                           usb_rate_stats[idx].sent_gap_10_20,
                           usb_rate_stats[idx].sent_gap_20_60,
                           usb_rate_stats[idx].sent_gap_60_75,
                           usb_rate_stats[idx].sent_gap_gt75);
            }
#endif
        }
        else
        {
#if OGXM_BT_RATE_DEBUG
            if (idx < MAX_GAMEPADS)
            {
                usb_rate_stats[idx].send_failed++;
            }
#endif
        }
    }
#if OGXM_BT_RATE_DEBUG
    else if (idx < MAX_GAMEPADS)
    {
        usb_rate_stats[idx].not_ready++;
    }
#endif
}

uint16_t SwitchDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    if (report_type != HID_REPORT_TYPE_INPUT || itf >= MAX_GAMEPADS)
    {
        return 0;
    }
    std::memcpy(buffer, &in_report_[itf], sizeof(SwitchWired::InReport));
	return sizeof(SwitchWired::InReport);
}

void SwitchDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool SwitchDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t* SwitchDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = reinterpret_cast<const char*>(SwitchWired::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* SwitchDevice::get_descriptor_device_cb() 
{
    return SwitchWired::DEVICE_DESCRIPTORS;
}

const uint8_t* SwitchDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return SwitchWired::REPORT_DESCRIPTORS;
}

const uint8_t* SwitchDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return SwitchWired::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* SwitchDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}
