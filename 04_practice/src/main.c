/** \file main.c */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED_NODE DT_CHOSEN(app_led)
#if !DT_NODE_EXISTS(LED_NODE)
#error "Missing /chosen node 'app-led'."
#endif

// The LED node's GPIO device specification. This information uses the property "gpios",
// which is of type `phandle-array`, where each phandle receives the `pin` and `flags` as metadata.
// Each field in the GPIO specification is in the end set using the `DT_PHA_BY_IDX` that we've
// seen in the devicetree semantics.
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

#define LED_NODE DT_CHOSEN(app_led)
#if !DT_NODE_EXISTS(LED_NODE)
#error "Missing /chosen node 'app-led'."
#endif

/*
 * For the GPIO subsystem, checking a node's status or using `gpio_is_ready_dt` currently
 * has the same effect, since the fields that are checked in the `gpio_is_ready_dt` function
 * are expanded by the preprocessor according to the node's status. However, in general
 * you should not assume that calling `is_ready`is equivalent to checking a node's status since,
 * e.g., `is_ready` might perform additional checks such as checking that an associated pin
 * is configured for the correct mode.
 */
#if !DT_NODE_HAS_STATUS(LED_NODE, okay)
#error "LED node status is not okay."
#endif

#define SLEEP_TIME_MS 500U

void main(void)
{
    int err = 0;

    /*
     * While devicetree macros are quite straightforward, for runtime functions such as
     * `gpio_is_ready_dt` the connection to the devicetree is sometimes harder to understand.
     * In case you're going down the rabbit hole trying to understand how `gpio_is_ready_dt`
     * connects to the node's `status` property, here's some tips:
     *
     * The GPIO subsystem is an instance based device driver. Such drivers use a fixed driver
     * model, explained in https://docs.zephyrproject.org/latest/kernel/drivers/index.html.
     * E.g., in `zephyr/drivers/gpio/gpio_nrfx.c`, currently at the very end of the file, you'll
     * find the following macro:
     *
     * `DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_DEVICE)`
     *
     * This macro ends up declaring instances `__device_dts_ord_<nn>` (and others) for each node
     * with the compatible property set to "nordic,nrf-gpio" and the status "okay". The node's
     * status is also reflected by the corresponding values written to the instance's fields, e.g.,
     * the field `device->state`.
     *
     * For "gpio.h", the function `gpio_is_ready_dt` in the end simply calls `z_device_is_ready`
     * with `spec->port`, which determines whether or not the device is ready as follows:
     *
     * `return dev->state->initialized && (dev->state->init_res == 0U);`
     *
     * To find the instances, you can, e.g., have a look at your linker file, where you might
     * find something like this:
     *
     * ```
     * .z_device_PRE_KERNEL_140_
     *    0x0000000000006534  0x30 zephyr/drivers/gpio/libdrivers__gpio.a(gpio_nrfx.c.obj)
     *    0x0000000000006534           __device_dts_ord_104
     *    0x000000000000654c           __device_dts_ord_11s
     * ```
     */
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
