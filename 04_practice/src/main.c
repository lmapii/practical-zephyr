/** \file main.c */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED_NODE DT_CHOSEN(app_led)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

#define SLEEP_TIME_MS 500U

void main(void)
{
    if (!gpio_is_ready_dt(&led))
    {
        return;
    }

    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) != 0)
    {
        return;
    }

    while (1)
    {
        (void) gpio_pin_toggle_dt(&led);
        k_msleep(SLEEP_TIME_MS);
    }
}
