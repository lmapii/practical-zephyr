
first:


TODO: initial setup could use QEMU ...

but hands on is better. most DKs will do but in this guide we're using a nrf52840 DK.

practical zephyr can be read back to back
it should give you enough information to find the documentation easy to read.

concept: practical zephyr as guide, read in official documentation, back to practical zephr.
basically a more extensive getting started guide
problem with zephr docs is that there is so much information that is not yet relevant when learning zephr.

ever changing but kconfig and devicetree are here to stay, so this guide aims to understand the fundamentals

more chapters?
- build optimizations (COMPILER_OPTIMIZATIONS Kconfig),
- build flags (activate well known/helpful warnings !!)
- testing with twister
- code analysis (new! code checker)
- virtual targets / emulation
- SWITCHING MCUs, is it really that easy?
- bootloaders and firmware updates
- matter
- [application version management](https://docs.zephyrproject.org/latest/build/version/index.html)
- custom boards

TODO: cleanup `CMake` vs. CMake and `Kconfig` etc.

vscode window size:
https://stackoverflow.com/a/68764796/7281683
window.resizeTo(1200, 1000);

terminal
Preferences>>Profiles>>[Profile we need to modify]>>Window
https://apple.stackexchange.com/a/429321
100x32

logging
https://blog.golioth.io/debugging-zephyr-for-beginners-printk-and-the-logging-subsystem/

https://blog.benjamin-cabe.com/2023/08/23/enabling-codechecker-for-your-zephyr-rtos-project

emulator also here
https://docs.zephyrproject.org/latest/develop/application/index.html#application-configuration-directory

https://github.com/zephyrproject-rtos/example-application
