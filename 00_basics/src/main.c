/** \file main.c */

#include <zephyr/kernel.h>

#define SLEEP_TIME_MS 100U

void main(void)
{
    // Zephyr is only of limited use without threads:
    // https://docs.zephyrproject.org/latest/kernel/services/threads/nothread.html#nothread
    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
