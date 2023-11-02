/** \file main.c */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED_NODE DT_CHOSEN(app_led)
#if !DT_NODE_EXISTS(LED_NODE)
#error "Missing /chosen node 'app-led'."
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

#define SLEEP_TIME_MS 500U

void main(void)
{
    int err = 0;

    //
    // zephyr/drivers/gpio/gpio_nrfx.c
    // - DEVICE_DT_INST_DEFINE
    //   state = &Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_ID(node_id))
    //
    // this is in turn read via this function, accessing the __device_dts_ord_<nn>
    // Device Driver Model: https://docs.zephyrproject.org/latest/kernel/drivers/index.html
    //
    // .z_device_PRE_KERNEL_140_
    //                0x0000000000006534       0x30 zephyr/drivers/gpio/libdrivers__gpio.a(gpio_nrfx.c.obj)
    //                0x0000000000006534                __device_dts_ord_104
    //                0x000000000000654c                __device_dts_ord_11s
    //
    if (!gpio_is_ready_dt(&led))
    {
        printk("Error: LED pin is not available.\n");
        return;
    }

    // TODO: MEMFAULT_ASSERT?
    err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (err != 0)
    {
        printk("Error %d: failed to configure LED pin.\n", err);
        return;
    }

    while (1)
    {
        (void) gpio_pin_toggle_dt(&led);
        k_msleep(SLEEP_TIME_MS);
    }
}
