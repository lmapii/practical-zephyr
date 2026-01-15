/** \file main.c */

#include <zephyr/kernel.h>

#ifdef CONFIG_USR_FUN
#include "usr_fun.h"
#endif

#define SLEEP_TIME_MS 100U

int main(void)
{
    printk("Message in a bottle.\n");

#ifdef CONFIG_USR_FUN
    usr_fun();
#endif

    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
