/** \file main.c */

#include <zephyr/kernel.h>

#define SLEEP_TIME_MS 100U

int main(void)
{
    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
