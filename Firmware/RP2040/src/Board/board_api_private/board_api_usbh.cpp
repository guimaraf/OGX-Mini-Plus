#include "Board/Config.h"
#if defined(CONFIG_EN_USB_HOST)

#include <atomic>
#include <hardware/gpio.h>
#include <pico/stdlib.h>

#include "Board/board_api_private/board_api_private.h"

namespace board_api_usbh {

std::atomic<bool> host_connected_ = false;

namespace
{
constexpr uint32_t HOST_IRQ_EVENTS = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
constexpr uint32_t HOST_POWER_CYCLE_DELAY_MS = 250;

void set_attach_irq_enabled(bool enabled)
{
    gpio_set_irq_enabled(PIO_USB_DP_PIN, HOST_IRQ_EVENTS, enabled);
    gpio_set_irq_enabled(PIO_USB_DP_PIN + 1, HOST_IRQ_EVENTS, enabled);
}

void sample_host_lines(bool rearm_irq_if_disconnected)
{
    const bool connected = gpio_get(PIO_USB_DP_PIN) || gpio_get(PIO_USB_DP_PIN + 1);
    host_connected_.store(connected);

    if (rearm_irq_if_disconnected)
    {
        set_attach_irq_enabled(!connected);
    }
}
}

void host_pin_isr(uint gpio, uint32_t events) {
    (void)events;

    set_attach_irq_enabled(false);

    if (gpio == PIO_USB_DP_PIN || gpio == PIO_USB_DP_PIN + 1) {
        sample_host_lines(true);
    }
}

bool host_connected() {
    sample_host_lines(true);
    return host_connected_.load();
}

void init() {
#if defined(VCC_EN_PIN)
    gpio_init(VCC_EN_PIN);
    gpio_set_dir(VCC_EN_PIN, GPIO_OUT);
    gpio_put(VCC_EN_PIN, 1);
#endif 

    gpio_init(PIO_USB_DP_PIN);
    gpio_set_dir(PIO_USB_DP_PIN, GPIO_IN);
    gpio_pull_down(PIO_USB_DP_PIN);

    gpio_init(PIO_USB_DP_PIN + 1);
    gpio_set_dir(PIO_USB_DP_PIN + 1, GPIO_IN);
    gpio_pull_down(PIO_USB_DP_PIN + 1);

    gpio_set_irq_enabled_with_callback(PIO_USB_DP_PIN, HOST_IRQ_EVENTS, false, &host_pin_isr);
    gpio_set_irq_enabled_with_callback(PIO_USB_DP_PIN + 1, HOST_IRQ_EVENTS, false, &host_pin_isr);
    sample_host_lines(true);
}

void recover_host_port() {
    host_connected_.store(false);
    set_attach_irq_enabled(false);

#if defined(VCC_EN_PIN)
    gpio_put(VCC_EN_PIN, 0);
    sleep_ms(HOST_POWER_CYCLE_DELAY_MS);
    gpio_put(VCC_EN_PIN, 1);
    sleep_ms(HOST_POWER_CYCLE_DELAY_MS);
#endif

    sample_host_lines(true);
}

} // namespace board_api_usbh

#endif // defined(CONFIG_EN_USB_HOST)
