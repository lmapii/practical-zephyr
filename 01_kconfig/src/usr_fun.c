#include "usr_fun.h"

#include <zephyr/kernel.h>

void usr_fun(void)
{
    printk("Message in a user function.\n");
}