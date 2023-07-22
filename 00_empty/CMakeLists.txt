cmake_minimum_required(VERSION 3.15~...3.20)

# The following is optional and effectively ignored if a different board is chosen as parameter.
# In general, it seems cleaner to specify the board when invoking west or CMake directly, or, e.g.,
# use `west config build.board nrf52840dk_nrf52840`.
# set(BOARD nrf52840dk_nrf52840)

# Resolve zephyr location using the ZEPHYR_BASE environment variable.
# The `REQUIRED HINTS $ENV{ZEPHYR_BASE}` is optional in case you're not using `west zephyr-export`
# https://docs.zephyrproject.org/latest/develop/west/zephyr-cmds.html#installing-cmake-packages-west-zephyr-export
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(
    EmptyApp
    VERSION 0.1
    DESCRIPTION "Empty zephyr application."
    LANGUAGES C
)

# The target "app" is defined within the Zephyr package by "find_package(Zephyr ...)".
target_sources(
    app
    PRIVATE
    src/main.c
)
