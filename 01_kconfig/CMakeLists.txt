cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(
    EmptyApp
    VERSION 0.1
    DESCRIPTION "Dummy Kconfig Zephyr application."
    LANGUAGES C
)

target_sources(
    app
    PRIVATE
    src/main.c
)

# The following shows how Kconfig symbols can be used to include or exclude files from the build:
target_sources_ifdef(
    CONFIG_USR_FUN
    app
    PRIVATE
    src/usr_fun.c
)