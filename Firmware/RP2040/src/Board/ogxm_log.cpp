#include "Board/Config.h"
#if defined(CONFIG_OGXM_DEBUG)

#include <cstdint>
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <pico/mutex.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>

#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"
#include "Board/ogxm_log.h"

std::ostream& operator<<(std::ostream& os, DeviceDriverType type) {
    switch (type) {
        case DeviceDriverType::NONE:          os << "NONE"; break;
        case DeviceDriverType::XBOXOG:        os << "XBOXOG"; break;
        case DeviceDriverType::XBOXOG_SB:     os << "XBOXOG_SB"; break;
        case DeviceDriverType::XBOXOG_XR:     os << "XBOXOG_XR"; break;
        case DeviceDriverType::XINPUT:        os << "XINPUT"; break;
        case DeviceDriverType::PS3:           os << "PS3"; break;
        case DeviceDriverType::DINPUT:        os << "DINPUT"; break;
        case DeviceDriverType::PSCLASSIC:     os << "PSCLASSIC"; break;
        case DeviceDriverType::SWITCH:        os << "SWITCH"; break;
        case DeviceDriverType::WEBAPP:        os << "WEBAPP"; break;
        case DeviceDriverType::UART_BRIDGE:   os << "UART_BRIDGE"; break;
        default:                              os << "UNKNOWN"; break;
    }
    return os;
}

namespace ogxm_log {

namespace {
constexpr size_t USB_LOG_BUFFER_SIZE = 2048;

std::array<uint8_t, USB_LOG_BUFFER_SIZE> usb_log_buffer{};
size_t usb_log_head = 0;
size_t usb_log_tail = 0;
size_t usb_log_count = 0;
mutex_t log_mutex;
bool log_mutex_initialized = false;

void ensure_log_mutex()
{
    if (!log_mutex_initialized)
    {
        mutex_init(&log_mutex);
        log_mutex_initialized = true;
    }
}

void push_usb_log_bytes(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0)
    {
        return;
    }

    if (size >= USB_LOG_BUFFER_SIZE)
    {
        data += size - USB_LOG_BUFFER_SIZE;
        size = USB_LOG_BUFFER_SIZE;
    }

    while ((usb_log_count + size) > USB_LOG_BUFFER_SIZE)
    {
        usb_log_tail = (usb_log_tail + 1) % USB_LOG_BUFFER_SIZE;
        --usb_log_count;
    }

    for (size_t i = 0; i < size; ++i)
    {
        usb_log_buffer[usb_log_head] = data[i];
        usb_log_head = (usb_log_head + 1) % USB_LOG_BUFFER_SIZE;
    }
    usb_log_count += size;
}
} // namespace

void init() {
    uart_init(DEBUG_UART_PORT, PICO_DEFAULT_UART_BAUD_RATE);
    gpio_set_function(PICO_DEFAULT_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PICO_DEFAULT_UART_RX_PIN, GPIO_FUNC_UART);
}

void log(const std::string& message) {
    ensure_log_mutex();

    mutex_enter_blocking(&log_mutex);

    std::string formatted_msg = "OGXM: " + message;

    uart_puts(DEBUG_UART_PORT, formatted_msg.c_str());
    push_usb_log_bytes(reinterpret_cast<const uint8_t*>(formatted_msg.data()), formatted_msg.size());

    mutex_exit(&log_mutex);
}

void log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    std::string formatted_msg = std::string(buffer);

    log(formatted_msg);

    va_end(args);
}

void log_hex(const uint8_t* data, size_t size) {
    std::ostringstream hex_stream;
    hex_stream << std::hex << std::setfill('0');
    int count = 0;
    for (uint16_t i = 0; i < size; ++i) {
        hex_stream << std::setw(2) << static_cast<int>(data[i]) << " ";
        if (++count == 16) {
            hex_stream << "\n";
            count = 0;
        }
    }
    hex_stream << "\n";
    log(hex_stream.str());
}

bool usb_log_available()
{
    ensure_log_mutex();
    mutex_enter_blocking(&log_mutex);
    const bool available = usb_log_count > 0;
    mutex_exit(&log_mutex);
    return available;
}

size_t read_usb_log(uint8_t* buffer, size_t size)
{
    if (buffer == nullptr || size == 0)
    {
        return 0;
    }

    ensure_log_mutex();
    mutex_enter_blocking(&log_mutex);

    const size_t bytes_to_read = std::min(size, usb_log_count);
    for (size_t i = 0; i < bytes_to_read; ++i)
    {
        buffer[i] = usb_log_buffer[usb_log_tail];
        usb_log_tail = (usb_log_tail + 1) % USB_LOG_BUFFER_SIZE;
    }
    usb_log_count -= bytes_to_read;

    mutex_exit(&log_mutex);
    return bytes_to_read;
}

} // namespace ogxm_log

#endif // defined(CONFIG_OGXM_DEBUG)
