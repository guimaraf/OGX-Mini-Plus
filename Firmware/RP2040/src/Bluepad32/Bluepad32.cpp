#include <atomic>
#include <cstring>
#include <functional>
#include <pico/mutex.h>
#include <pico/cyw43_arch.h>
#include <pico/time.h>

#include "btstack_run_loop.h"
#include "uni.h"

#include "sdkconfig.h"
#include "Bluepad32/Bluepad32.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
    #error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

static_assert((CONFIG_BLUEPAD32_MAX_DEVICES == MAX_GAMEPADS), "Mismatch between BP32 and Gamepad max devices");

namespace bluepad32 {

static constexpr uint32_t FEEDBACK_TIME_MS = 250;
static constexpr uint32_t LED_CHECK_TIME_MS = 500;

#ifndef OGXM_BT_RATE_DEBUG
#define OGXM_BT_RATE_DEBUG 0
#endif

struct BTDevice {
    bool connected{false};
    Gamepad* gamepad{nullptr};
};

BTDevice bt_devices_[MAX_GAMEPADS];
btstack_timer_source_t feedback_timer_;
btstack_timer_source_t led_timer_;
bool led_timer_set_{false};
bool feedback_timer_set_{false};

#if OGXM_BT_RATE_DEBUG
struct BtRateStats {
    uint32_t window_start_ms{0};
    uint32_t callbacks{0};
    uint32_t gamepad_callbacks{0};
    uint32_t pad_updates{0};
    uint32_t invalid_index{0};
    uint32_t not_connected{0};
    uint32_t last_pad_update_ms{0};
    uint32_t pad_gap_min_ms{0};
    uint32_t pad_gap_max_ms{0};
    uint32_t pad_gap_lt5{0};
    uint32_t pad_gap_lt10{0};
    uint32_t pad_gap_10_20{0};
    uint32_t pad_gap_20_60{0};
    uint32_t pad_gap_60_75{0};
    uint32_t pad_gap_gt75{0};
};

BtRateStats bt_rate_stats_[MAX_GAMEPADS];

static const char* controller_type_name(uni_controller_type_t type)
{
    switch (type)
    {
        case CONTROLLER_TYPE_XBoxOneController:
            return "xbox_one";
        case CONTROLLER_TYPE_PS4Controller:
            return "ps4";
        case CONTROLLER_TYPE_PS5Controller:
            return "ps5";
        case CONTROLLER_TYPE_SwitchProController:
            return "switch_pro";
        case CONTROLLER_TYPE_SwitchJoyConLeft:
            return "joycon_l";
        case CONTROLLER_TYPE_SwitchJoyConRight:
            return "joycon_r";
        case CONTROLLER_TYPE_AndroidController:
            return "android";
        default:
            return "other";
    }
}

static void log_bt_rate_stats(uint8_t idx, uni_hid_device_t* device)
{
    BtRateStats& stats = bt_rate_stats_[idx];
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

    OGXM_LOG("[BT_RATE] idx=%u type=%s cb=%lu gp=%lu set=%lu bad_idx=%lu not_conn=%lu elapsed=%lu\n",
             idx,
             controller_type_name(device->controller_type),
             stats.callbacks,
             stats.gamepad_callbacks,
             stats.pad_updates,
             stats.invalid_index,
             stats.not_connected,
             elapsed_ms);

    OGXM_LOG("[BT_GAP] idx=%u type=%s set_min=%lu set_max=%lu lt5=%lu lt10=%lu 10_20=%lu 20_60=%lu 60_75=%lu gt75=%lu elapsed=%lu\n",
             idx,
             controller_type_name(device->controller_type),
             stats.pad_gap_min_ms,
             stats.pad_gap_max_ms,
             stats.pad_gap_lt5,
             stats.pad_gap_lt10,
             stats.pad_gap_10_20,
             stats.pad_gap_20_60,
             stats.pad_gap_60_75,
             stats.pad_gap_gt75,
             elapsed_ms);

    stats = BtRateStats{};
    stats.window_start_ms = now_ms;
}

static void record_bt_pad_gap(BtRateStats& stats)
{
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if (stats.last_pad_update_ms != 0)
    {
        uint32_t gap_ms = now_ms - stats.last_pad_update_ms;
        if (stats.pad_gap_min_ms == 0 || gap_ms < stats.pad_gap_min_ms)
        {
            stats.pad_gap_min_ms = gap_ms;
        }
        if (gap_ms > stats.pad_gap_max_ms)
        {
            stats.pad_gap_max_ms = gap_ms;
        }

        if (gap_ms < 5)
            stats.pad_gap_lt5++;
        else if (gap_ms < 10)
            stats.pad_gap_lt10++;
        else if (gap_ms < 20)
            stats.pad_gap_10_20++;
        else if (gap_ms < 60)
            stats.pad_gap_20_60++;
        else if (gap_ms <= 75)
            stats.pad_gap_60_75++;
        else
            stats.pad_gap_gt75++;
    }

    stats.last_pad_update_ms = now_ms;
}
#endif

bool any_connected()
{
    for (auto& device : bt_devices_)
    {
        if (device.connected)
        {
            return true;
        }
    }
    return false;
}

//This solves a function pointer/crash issue with bluepad32
void set_rumble(uni_hid_device_t* bp_device, uint16_t length, uint8_t rumble_l, uint8_t rumble_r)
{
    switch (bp_device->controller_type)
    {
        case CONTROLLER_TYPE_XBoxOneController:
            uni_hid_parser_xboxone_play_dual_rumble(bp_device, 0, length + 10, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_AndroidController:
            if (bp_device->vendor_id == UNI_HID_PARSER_STADIA_VID && bp_device->product_id == UNI_HID_PARSER_STADIA_PID) 
            {
                uni_hid_parser_stadia_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            }
            break;
        case CONTROLLER_TYPE_PSMoveController:
            uni_hid_parser_psmove_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_PS3Controller:
            uni_hid_parser_ds3_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_PS4Controller:
            uni_hid_parser_ds4_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_PS5Controller:
            uni_hid_parser_ds5_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_WiiController:
            uni_hid_parser_wii_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_SwitchProController:
        case CONTROLLER_TYPE_SwitchJoyConRight:
        case CONTROLLER_TYPE_SwitchJoyConLeft:
            uni_hid_parser_switch_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        default:
            break;
    }
}

static void send_feedback_cb(btstack_timer_source *ts)
{
    uni_hid_device_t* bp_device = nullptr;

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        if (!bt_devices_[i].connected || 
            !(bp_device = uni_hid_device_get_instance_for_idx(i)))
        {
            continue;
        }

        Gamepad::PadOut gp_out = bt_devices_[i].gamepad->get_pad_out();
        if (gp_out.rumble_l > 0 || gp_out.rumble_r > 0)
        {
            set_rumble(bp_device, static_cast<uint16_t>(FEEDBACK_TIME_MS), gp_out.rumble_l, gp_out.rumble_r);
        }
    }

    btstack_run_loop_set_timer(ts, FEEDBACK_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

static void check_led_cb(btstack_timer_source *ts)
{
    static bool led_state = false;

    led_state = !led_state;

    board_api::set_led(any_connected() ? true : led_state);

    btstack_run_loop_set_timer(ts, LED_CHECK_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

//BT Driver

static void init(int argc, const char** arg_V) {
}

static void init_complete_cb(void) {
    uni_bt_enable_new_connections_unsafe(true);
    // uni_bt_del_keys_unsafe();
    uni_property_dump_all();
}

static uni_error_t device_discovered_cb(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    if (!((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_GAMEPAD)) {
        return UNI_ERROR_IGNORE_DEVICE;
    }
    return UNI_ERROR_SUCCESS;
}

static void device_connected_cb(uni_hid_device_t* device) {
}

static void device_disconnected_cb(uni_hid_device_t* device) {
    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= MAX_GAMEPADS || idx < 0) {
        return;
    }

    bt_devices_[idx].connected = false;
    bt_devices_[idx].gamepad->reset_pad_in();

    if (!led_timer_set_ && !any_connected()) {
        led_timer_set_ = true;
        led_timer_.process = check_led_cb;
        led_timer_.context = nullptr;
        btstack_run_loop_set_timer(&led_timer_, LED_CHECK_TIME_MS);
        btstack_run_loop_add_timer(&led_timer_);
    }
    if (feedback_timer_set_ && !any_connected()) {
        feedback_timer_set_ = false;
        btstack_run_loop_remove_timer(&feedback_timer_);
    }
}

static uni_error_t device_ready_cb(uni_hid_device_t* device) {    
    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= MAX_GAMEPADS || idx < 0) {
        return UNI_ERROR_SUCCESS;
    }

    bt_devices_[idx].connected = true;

    if (led_timer_set_) {
        led_timer_set_ = false;
        btstack_run_loop_remove_timer(&led_timer_);
        board_api::set_led(true);
    }
    if (!feedback_timer_set_) {
        feedback_timer_set_ = true;
        feedback_timer_.process = send_feedback_cb;
        feedback_timer_.context = nullptr;
        btstack_run_loop_set_timer(&feedback_timer_, FEEDBACK_TIME_MS);
        btstack_run_loop_add_timer(&feedback_timer_);
    }
    return UNI_ERROR_SUCCESS;
}

static void oob_event_cb(uni_platform_oob_event_t event, void* data) {
	return;
}

static void controller_data_cb(uni_hid_device_t* device, uni_controller_t* controller) {
    static uni_gamepad_t prev_uni_gp[MAX_GAMEPADS] = {};

#if OGXM_BT_RATE_DEBUG
    int raw_idx = uni_hid_device_get_idx_for_instance(device);
    if (raw_idx < 0 || raw_idx >= MAX_GAMEPADS)
    {
        static BtRateStats invalid_stats;
        invalid_stats.callbacks++;
        invalid_stats.invalid_index++;
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if (invalid_stats.window_start_ms == 0)
        {
            invalid_stats.window_start_ms = now_ms;
        }
        else if ((now_ms - invalid_stats.window_start_ms) >= 1000)
        {
            OGXM_LOG("[BT_RATE] idx=invalid cb=%lu bad_idx=%lu elapsed=%lu\n",
                     invalid_stats.callbacks,
                     invalid_stats.invalid_index,
                     now_ms - invalid_stats.window_start_ms);
            invalid_stats = BtRateStats{};
            invalid_stats.window_start_ms = now_ms;
        }
        return;
    }

    BtRateStats& stats = bt_rate_stats_[raw_idx];
    stats.callbacks++;
#endif

    if (controller->klass != UNI_CONTROLLER_CLASS_GAMEPAD){
#if OGXM_BT_RATE_DEBUG
        log_bt_rate_stats(static_cast<uint8_t>(raw_idx), device);
#endif
        return;
    }

    uni_gamepad_t *uni_gp = &controller->gamepad;
    int idx = uni_hid_device_get_idx_for_instance(device);

#if OGXM_BT_RATE_DEBUG
    stats.gamepad_callbacks++;
    if (!bt_devices_[idx].connected)
    {
        stats.not_connected++;
    }
#endif

    Gamepad* gamepad = bt_devices_[idx].gamepad;
    Gamepad::PadIn gp_in;

    switch (uni_gp->dpad) 
    {
        case DPAD_UP:
            gp_in.dpad = gamepad->MAP_DPAD_UP;
            break;
        case DPAD_DOWN:
            gp_in.dpad = gamepad->MAP_DPAD_DOWN;
            break;
        case DPAD_LEFT:
            gp_in.dpad = gamepad->MAP_DPAD_LEFT;
            break;
        case DPAD_RIGHT:
            gp_in.dpad = gamepad->MAP_DPAD_RIGHT;
            break;
        case DPAD_UP | DPAD_RIGHT:
            gp_in.dpad = gamepad->MAP_DPAD_UP_RIGHT;
            break;
        case DPAD_DOWN | DPAD_RIGHT:
            gp_in.dpad = gamepad->MAP_DPAD_DOWN_RIGHT;
            break;
        case DPAD_DOWN | DPAD_LEFT:
            gp_in.dpad = gamepad->MAP_DPAD_DOWN_LEFT;
            break;
        case DPAD_UP | DPAD_LEFT:
            gp_in.dpad = gamepad->MAP_DPAD_UP_LEFT;
            break;
        default:
            break;
    }

    if (uni_gp->buttons & BUTTON_A) gp_in.buttons |= gamepad->MAP_BUTTON_A;
    if (uni_gp->buttons & BUTTON_B) gp_in.buttons |= gamepad->MAP_BUTTON_B;
    if (uni_gp->buttons & BUTTON_X) gp_in.buttons |= gamepad->MAP_BUTTON_X;
    if (uni_gp->buttons & BUTTON_Y) gp_in.buttons |= gamepad->MAP_BUTTON_Y;
    if (uni_gp->buttons & BUTTON_SHOULDER_L) gp_in.buttons |= gamepad->MAP_BUTTON_LB;
    if (uni_gp->buttons & BUTTON_SHOULDER_R) gp_in.buttons |= gamepad->MAP_BUTTON_RB;
    if (uni_gp->buttons & BUTTON_THUMB_L)    gp_in.buttons |= gamepad->MAP_BUTTON_L3;  
    if (uni_gp->buttons & BUTTON_THUMB_R)    gp_in.buttons |= gamepad->MAP_BUTTON_R3;
    if (uni_gp->misc_buttons & MISC_BUTTON_BACK)    gp_in.buttons |= gamepad->MAP_BUTTON_BACK;
    if (uni_gp->misc_buttons & MISC_BUTTON_START)   gp_in.buttons |= gamepad->MAP_BUTTON_START;
    if (uni_gp->misc_buttons & MISC_BUTTON_SYSTEM)  gp_in.buttons |= gamepad->MAP_BUTTON_SYS;

    gp_in.trigger_l = gamepad->scale_trigger_l<10>(static_cast<uint16_t>(uni_gp->brake));
    gp_in.trigger_r = gamepad->scale_trigger_r<10>(static_cast<uint16_t>(uni_gp->throttle));

    if (device->controller_type == CONTROLLER_TYPE_SwitchProController ||
        device->controller_type == CONTROLLER_TYPE_SwitchJoyConLeft ||
        device->controller_type == CONTROLLER_TYPE_SwitchJoyConRight) 
    {
        if (uni_gp->buttons & BUTTON_TRIGGER_L) gp_in.trigger_l = 255;
        if (uni_gp->buttons & BUTTON_TRIGGER_R) gp_in.trigger_r = 255;
    }
    
    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad->scale_joystick_l<10>(uni_gp->axis_x, uni_gp->axis_y);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad->scale_joystick_r<10>(uni_gp->axis_rx, uni_gp->axis_ry);

    gamepad->set_pad_in(gp_in);

#if OGXM_BT_RATE_DEBUG
    record_bt_pad_gap(stats);
    stats.pad_updates++;
    log_bt_rate_stats(static_cast<uint8_t>(idx), device);
#endif
}

const uni_property_t* get_property_cb(uni_property_idx_t idx) 
{
    return nullptr;
}

uni_platform* get_driver() 
{
    static uni_platform driver = 
    {
        .name = "OGXMiniW",
        .init = init,
        .on_init_complete = init_complete_cb,
        .on_device_discovered = device_discovered_cb,
        .on_device_connected = device_connected_cb,
        .on_device_disconnected = device_disconnected_cb,
        .on_device_ready = device_ready_cb,
        .on_controller_data = controller_data_cb,
        .get_property = get_property_cb,
        .on_oob_event = oob_event_cb,
    };
    return &driver;
}

//Public API

void run_task(Gamepad(&gamepads)[MAX_GAMEPADS])
{
    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        bt_devices_[i].gamepad = &gamepads[i];
    }

    uni_platform_set_custom(get_driver());
    uni_init(0, nullptr);

    led_timer_set_ = true;
    led_timer_.process = check_led_cb;
    led_timer_.context = nullptr;
    btstack_run_loop_set_timer(&led_timer_, LED_CHECK_TIME_MS);
    btstack_run_loop_add_timer(&led_timer_);

    btstack_run_loop_execute();
}

} // namespace bluepad32
