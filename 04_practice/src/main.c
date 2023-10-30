/** \file main.c */

#include <zephyr/kernel.h>

#define SLEEP_TIME_MS 100U

void main(void)
{
    printk("Message in a bottle.\n");

    // TODO: MEMFAULT_ASSERT() for failed status

    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
