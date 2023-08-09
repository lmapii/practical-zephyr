
Logging as demo for Kconfig

- [Goal](#goal)
- [Getting started using `printk`](#getting-started-using-printk)
- [Kconfig](#kconfig)
  - [Configuring symbols](#configuring-symbols)
- [Navigating Kconfig](#navigating-kconfig)
  - [Loading `menuconfig` and `guiconfig`](#loading-menuconfig-and-guiconfig)
  - [Persisting `menuconfig` and `guiconfig` changes](#persisting-menuconfig-and-guiconfig-changes)
  - [Kconfig search](#kconfig-search)
  - [Nordic Kconfig extension for `vscode`](#nordic-kconfig-extension-for-vscode)
- [Using different configuration files](#using-different-configuration-files)
  - [Board Kconfig fragments](#board-kconfig-fragments)
  - [Build types](#build-types)
  - [Extra configuration files](#extra-configuration-files)
- [Kconfig hardening](#kconfig-hardening)
- [Overlays and build configurations](#overlays-and-build-configurations)
- [A custom Kconfig symbol](#a-custom-kconfig-symbol)
- [Logging](#logging)

## Goal

In this section, we'll explore the [Kconfig configuration system][zephyr-kconfig] by looking at the `printk` logging option in Zephyr. [_Logging_][zephyr-logging] in Zephyr is everything from simple []`print`-style text logs][zephyr-printk] to customized messaging. Notice, though, that we'll _not_ explore the logging service in detail, but only use it as an excuse to dig into [Kconfig][zephyr-kconfig]. Finally, we'll create our own little application specific `Kconfig` configuration.

## Getting started using `printk`

Go ahead and create a new, empty project, e.g., using the empty application from the [previous section](../00_empty/readme.md).

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

The simplest way to log text in Zephyr is the `printk` function. The `printk` also takes multiple arguments and format strings, but that's not what we want to show here, so we'll just modify the `main` functino to output the string _"Message in a bottle."_ each time it is called.

```c
#include <zephyr/kernel.h>
#define SLEEP_TIME_MS 100U

void main(void)
{
    printk("Message in a bottle.\n");
    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
```

But where is this text going to?

As mentioned in the [previous section](../00_empty/readme.md), in this guide we're using a development kit from [Nordic][nordicsemi]; in my case the [development kit for the nRF52840][nordicsemi-nrf52840-dk]. For most development kits the logging output is available via its UART interface and using the following settings:

* Baud rate: _115200_
* Data bits: _8_
* No parity, one stop bit.

The connection settings for the UART interface are configured using the [Devicetree][zephyr-dts]. We'll explore the devicetree in later sections. Since these default settings are all we need for now, let's focus on Kconfig and go ahead and build and flash the project:

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build
$ west flash --build-dir ../build
```

When connecting the development kit to my computer, it shows up as TTY device `/dev/tty.usbmodem<some-serial-number>`. The format may vary depending on your development kit and operating system, e.g., on Windows it'll show up as a `COM` port.

Open your serial monitor of choice, e.g., using the `screen` command, by launching [PuTTY][serial-putty] or - in case you're using _macos_ - [Serial][serial-dt], and reboot or reflash your device. You should see the following output each time your device starts:

```
$ screen /dev/tty.usbmodem<some-serial-number> 115200
*** Booting Zephyr OS build v3.3.99-ncs1 ***
Message in a bottle.

$ # use CTRL+A+k to quit the screen command.
```

Great! But what if we'd like to get rid of this feature entirely without changing our code? We all know that string processing is expensive and slow on an embedded system. Let's assume that in production we want to get rid of it entirely to reduce our image size and to speed up our application. How do we achieve this? This is where the [Kconfig configuration system][zephyr-kconfig] comes into play.



## Kconfig

[So far](../00_empty/readme.md) we've seen that Zephyr uses [CMake][zephyr-cmake] as one of its [build and configuration systems][zephyr-build]. On top of CMake, Zephyr - just like the Linux kernel - uses the [Kconfig configuration system][zephyr-kconfig]. We've already encountered Kconfig in the [previous section](../00_empty/readme.md), where we had to provide the required _application configuration file_ [prj.conf](../00_empty/prj.conf).

In this section we'll only explore the practical aspects of Kconfig. Details about Kconfig in Zephyr can be found in the [official documentation][zephyr-kconfig]. So, what is Kconfig?

In simple words, Kconfig is a configuration system that uses a hierarchy of `Kconfig` configuration _files_ (with their own syntax) which in turn define a hierarchy of configuration _options_, also called _symbols_. These symbols are used by the _build system_ to include or exclude files from the build process, and as configuration options in the _source code_ itself. All in all, this allows you to modify your application without having to modify the source code.

### Configuring symbols

For the time being, let's ignore _how_ the entire `Kconfig` hierarchy is built or merged and lets just have a look at how the configuration system works in general. We'll touch the file handling in a later chapter.

The debugging subsystem of Zephyr defines a `Kconfig` symbol allowing you to enable or disable the `printk` output. The `PRINTK` _symbol_ is defined in the `Kconfig` file `zephyr/subsys/debug/Kconfig`. We won't go into detail about the syntax here, for this simple example it is quite self explanatory. The `Kconfig` language is specified in the [Linux kernel documentation][kernel-config], Zephr's documentation contains a good section of [Kconfig tips and best practices][zephyr-kconfig-tips].

```conf
config PRINTK
	bool "Send printk() to console"
	default y
```

Unless configured otherwise, `PRINTK` is therefore *enabled* by default. We can configure symbols to our needs in the _application configuration file_ [prj.conf](../00_empty/prj.conf). Symbols are assigned their values using the following syntax. Notice that at the time of writing no spaces are allowed before or after the `=` operator:

```conf
CONFIG_<symbol name>=<value>
```

Our `PRINTK` is a symbol of type `bool` and can therefore be assigned the values `y` and `n` (have a look at the [official documentation][zephyr-kconfig-syntax] for details). To disable `PRINTK`, we can therefore add the following to our application configuration file [prj.conf](../00_empty/prj.conf):

```conf
CONFIG_PRINTK=n
```

What's the effect of changing a symbol? As mentioned before, this _can_ have an effect on the build options and _can_ also be used by some subsystem's implementation. The effect, however, is specific for each symbol, meaning there is no convention for symbol names that will always have the same effect. Thus, in the end it is up to you to check which options are available and what effect they have.

Let's have a short look at our `PRINTK` symbol. A quick search for `CONFIG_PRINTK` reveals how the symbol is used in the implementation of the `printk` debugging subsystem. Notice that the below snippets are simplified versions of the real code:

`zephyr/lib/os/printk.h`
```c
#ifdef CONFIG_PRINTK
extern void printk(const char *fmt, ...);
#else
static inline void printk(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
}
```

`zephyr/lib/os/printk.c`
```c
#ifdef CONFIG_PRINTK
void printk(const char *fmt, ...)
{
  // ...
}
#endif /* defined(CONFIG_PRINTK) */
```

> **Notice:** In our `prj.conf` file we **set** `CONFIG_PRINTK=n`, but in the implementation this translates to a missing definition, not a definition with a value of `n`. This is simply what a `bool` symbol translates to in the implementation. For details I'll once again have to direct you to the [official documentation][zephyr-kconfig-syntax]. We'll just accept this as a given here.

In the header file we can see that `printk` is replaced by a dummy function in case `PRINTK` is disabled. Thus, all the calls to `printk` will effectively be removed by the compiler. In the `printk.c` source file we can see that the the `#if` directive is used to only conditionally compile the relevant source code in case `PRINTK` is enabled. Later in this section, we'll also see an example of how files can be excluded by the build system itself in case an option is not enabled.

With `printk` disabled in our application configuration file, let's try to rerun our application to verify that the output is indeed disabled:

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build --pristine
$ west flash --build-dir ../build
```

> **Notice:** When changing your `Kconfig` symbols, it is recommended to use the `--pristine` flag to enforce a complete rebuild of your application. This then also requires to specify the correct `--board`.

```
$ screen /dev/tty.usbmodem<some-serial-number> 115200
*** Booting Zephyr OS build v3.3.99-ncs1 ***
Message in a bottle.
```

Huh. Even though we've disabled `PRINTK` using `CONFIG_PRINTK=n` in our `prj.conf` file, the output is still not disabled. The build passed, there were no warnings - did we miss something?



## Navigating Kconfig

As mentioned before, `Kconfig` uses an entire _hierarchy_ of `Kconfig` files. The initial configuration is merged from several `Kconfig` files, including the `Kconfig` file of the specified board. The exact procedure used by Zephyr for merging the configuration file is explained in great detail in a [dedicated section of the official documentation][zephyr-kconfig-merge].

When debugging `Kconfig` settings, it can sometimes be helpful to have a look at this generated output. All `Kconfig` configuration parameters are merged into a single `zephyr/.config` file located in the build directory, in our case `build/zephyr/.config`. There, we find the following setting:

```conf
#
# Debugging Options
#
# CONFIG_DEBUG is not set
# CONFIG_STACK_USAGE is not set
# CONFIG_STACK_SENTINEL is not set
CONFIG_PRINTK=y
```

Thus, it seems that our `CONFIG_PRINTK` setting was not accepted. Why? `Kconfig` files can have _dependencies_: Enabling one option can therefore automatically also enable other options. Sadly, at the time of writing, `Kconfig` does not seem to warn about conflicting symbols, e.g., from the application configuration file.

Zephyr contains _hundreds_ of so called `Kconfig` _fragments_. It is therefore almost impossible to navigate these files without tool support. Thankfully, `west` integrates calls to two graphical tools to explore the `Kconfig` options: `menuconfig` and `guiconfig`.

### Loading `menuconfig` and `guiconfig`

For the `build` command, `west` has some builtin _targets_, two of which are used for `Kconfig`:

```bash
$ west build --build-dir ../build -t usage
-- west build: running target usage
...
Kconfig targets:
  menuconfig - Update .config using a console-based interface
  guiconfig  - Update .config using a graphical interface
...
```

The targets `menuconfig` and `guiconfig` can thus be used to start graphical `Kconfig` tools to modify the `.config` file in your build directory. Both tools are essentially identical. The only difference is that `menuconfig` is a text-based graphical interface that opens directly in your terminal, whereas `guiconfig` is a "clickable" user interface running in its own window:

```bash
$ west build --build-dir ../build -t menuconfig
$ west build --build-dir ../build -t guiconfig
```

> **Notice:** If you did not build your project before and if the project does not compile, then the tool will not load since the `zephyr/.config` file is missing.

![Screenshot menuconfig](../assets/kconfig-menuconfig.png?raw=true "menuconfig")
![Screenshot guiconfig](../assets/kconfig-guiconfig.png?raw=true "guiconfig")

For the sake of simplicity we'll use `menuconfig`. In both tools, you can search for configuration symbols. In `menuconfig` - similar to `vim` - you can use the forward slash `/` to start searching, e.g., for our `PRINTK` symbol. Several results match, but only one of them is the plain symbol `PRINTK` that also shows the same description that we've seen in the `Kconfig` file:

```
PRINTK(=y) "Send printk() to console"
```

Navigate to the symbol either using the search or using the path `Subsystems and OS Services > Debugging Options`. You should see the following screen:

![Screenshot menuconfig-debug](../assets/kconfig-menuconfig-debug.png?raw=true "menuconfig Debuggin Options")

Typically, you can toggle options using `Space` or `Enter`, but this does not seem to be possible for `PRINTK`. You can see that a symbol cannot be changed since it is enclosed by two dashes `-*-` instead of square brackets `[*]`. Using `?` we can display the symbol information:

```
Name: PRINTK
Prompt: Send printk() to console
Type: bool
Value: y

Help:

  This option directs printk() debugging output to the supported
  console device, rather than suppressing the generation
  of printk() output entirely. Output is sent immediately, without
  any mutual exclusion or buffering.

Default:
  - y

Symbols currently y-selecting this symbol:
  - BOOT_BANNER
...
```

We found the problem! The symbol `BOOT_BANNER` is `y-selecting` the `PRINTK` symbol, which is why we can't disable it and why the application configuration has no effect. This should also make you realize that it is hard if not impossible to figure out the dependency by navigating `Kconfig` fragments. Let's fix our configuration:

* Navigate to the `BOOT_BANNER` symbol and disable it.
* Going back to `PRINTK` it should now be possible to disable it.


### Persisting `menuconfig` and `guiconfig` changes

`menuconfig` and `guiconfig` can be used to test and explore configuration options. Typically, you'd use the normal _Save_ operation to store the new configuration to the `zephyr/.config` file in your build directory. You can then also launch `menuconfig` multiple times and change the configuration to your needs, and the changes will always be reflected in the next build without having to perform a _pristine_ build.

On the other hand, when saving to `zephyr/.config` in the build directory, **all changes will be lost** once you perform a clean build. To persist your changes, you need to place them into your [configuration files][zephyr-kconfig-syntax].

**Using `diff`**

The people at [Golioth][golioth] published a [short article][golioth-kconfig-diff] that shows how to leverage the `diff` command find out which configuration options have been changed when using the normal _Save_ operation: Before writing the changes to the `zephyr/.config` file, Kconfig stores the old configuration in `build/zephyr/.config.old`. Thus, all changes that you do between a _Save_ operation are available in the differences between `zephyr/.config` and `build/zephyr/.config.old`:

```bash
$ west build --board nrf52840dk_nrf52840 -d ../build --pristine
$ west build -d ../build -t menuconfig
# Within menuconfig:
# - Disable BOOT_BANNER and PRINTK.
# - Save the changes to ../build/zephyr/.config using [S].

$ diff ../build/zephyr/.config ../build/zephyr/.config.old
1097c1097
< # CONFIG_BOOT_BANNER is not set
---
> CONFIG_BOOT_BANNER=y
1462c1462
< # CONFIG_PRINTK is not set
---
> CONFIG_PRINTK=y
```

> **Notice:** The `# CONFIG_<symbol> is not set` comment syntax is used instead of `CONFIG_<symbol>=n` for historical reasons, as also mentioned in the [official documentation][zephyr-kconfig-syntax]. This allows parsing configuration files directly as `makefiles`.

You can now simply take the changes from the `diff` and persist them, e.g., by writing them into your application configuration file. Keep in mind, however, that this only works for changes performed between one _Save_ operation. If you _Save_ multiple times, `.config.old` is always replaced by the current `.config`. The `diff` operation must therefore be performed after each _Save_.

This approach, however, is not always feasible. E.g., try to enable the logging subsystem `Subsystems and OS Services > Logging`: You'll notice that the `diff` is huge even though you've only changed one option. This is due to the large number of dependencies of the `CONFIG_LOG=y` setting.

**Save minimal configuration**

Both, `menuconfig` and `guiconfig`, have the _Save minimal config_ option. As the name implies, this option exports a minimal `Kconfig` configuration containing _all_ symbols that differ from their default value. This does, however, also mean that symbols that differ from their default values due to other `Kconfig` fragments, e.g., the symbols in the `Kconfig` files of the chosen board, will be saved in this minimal configuration file.

Let's try this out with our settings for the `BOOT_BANNER` and `PRINTK` symbols.

```bash
$ west build --board nrf52840dk_nrf52840 -d ../build --pristine
$ west build -d ../build -t menuconfig
# Within menuconfig:
# - Disable BOOT_BANNER and PRINTK.
# - Use [D] "Save minimal config" and write the changes to tmp.conf.
$ cat tmp.conf
```
```conf
CONFIG_GPIO=y
CONFIG_PINCTRL=y
CONFIG_SERIAL=y
CONFIG_NRFX_GPIOTE_NUM_OF_EVT_HANDLERS=1
CONFIG_USE_SEGGER_RTT=y
CONFIG_SOC_SERIES_NRF52X=y
CONFIG_SOC_NRF52840_QIAA=y
CONFIG_ARM_MPU=y
CONFIG_HW_STACK_PROTECTION=y
# CONFIG_BOOT_BANNER is not set
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
# CONFIG_PRINTK is not set
CONFIG_EARLY_CONSOLE=y
```

As you can see, this minimal configuration contains a lot more options than we changed within `menuconfig`. Some of these options come from the selected board, e.g., `zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig`, but are still listed since they do not use default values. However, the two changes to `PRINTK` and `BOOT_BANNER` are still visible. We can just take those options and write them to our application configuration file:

```bash
$ cat prj.conf
CONFIG_PRINTK=n
CONFIG_BOOT_BANNER=n
```

Now, after rebuilding, the output on our serial interface indeed remains empty. Also, if you look at the output of your build should notice that the _flash_ usage decreased by around 2 KB.

### Kconfig search

In case you don't like `menuconfig` or `guiconfig`, or just want to browse available configuration options, the Zephyr documentation also includes a dedicated [Kconfig search][zephyr-kconfig-search]. E.g., when [searching for our `PRINTK` configuration](https://docs.zephyrproject.org/latest/kconfig.html#CONFIG_PRINTK) we'll be presented with the following output:

![Screenshot Kconfig Search](../assets/kconfig-search.png?raw=true "Kconfig search")
 the search produces the same information as available in `menuconfig`, but _without_ knowing our current conf

While this search does point out that there is a dependency to the `BOOT_BANNER` symbol, it cannot know our current configuration and therefore can't tell us that we can't only disable `PRINTK` since `BOOT_BANNER` is also enabled in our configuration.


### Nordic Kconfig extension for `vscode`

Another graphical user interface for `Kconfig` is the [nRF Kconfig][nrf-vscode-kconfig] extension for `vscode`. To some extent this extension can also be used without all other extensions of the [nRF Connect SDK][nrf-connect-sdk] and can therefore also be useful if you're not developing for a target from [Nordic][nordicsemi]:

![Screenshot nRF Kconfig](../assets/kconfig-nrf-vscode.png?raw=true "nRF Kconfig")

At the time of writing and for this repository the following configuration options were necessary to use the extension in workspace that does not use the entire extension set:

```json
{
  "nrf-connect.toolchain.path": "${nrf-connect.toolchain:2.4.0}",
  "nrf-connect.topdir": "${nrf-connect.sdk:2.4.0}",
  "kconfig.zephyr.base": "${nrf-connect.sdk:2.4.0}",
  "nrf-connect.applications": ["${workspaceFolder}/path/to/app"]
}
```

However, as visible in the above screenshot, at the time of writing the extension fails to inform the user about the dependency to the `BOOT_BANNER` and in contrast to `menuconfig` and `guiconfig` it is therefore not visible why the configuration option cannot be disabled.

Aside from that, this extension has two major benefits:

* The _Save minimal config_ option seems to recognize options coming from a `_defconfig` file and therefore really only exports the configuration options set within the extension.
* The extension adds auto-completion to your `.conf` files.

With this last tool to explore `Kconfig`, let's have a look at a couple of more well used options.



## Using different configuration files

TODO: here. until now: only prj.conf, but there's more and there's some common practices.

### Board Kconfig fragments

prj_<board>.conf -> deprecated, use /board folder
TODO: disable GPIO in this overlay and see that it is indeed disabled

### Build types

`release` or `debug` are not supported build types in Zephyr.

[docs][zephyr-dconf]

Default: prj.conf
```bash
west build --board nrf52840dk_nrf52840 -d ../build -- -DCONF_FILE=prj_release.conf
```

### Extra configuration files

EXTRA_CONF_FILE replaces OVERLAY_CONFIG
hidden in the [important build system variables][zephyr-kconfig-extra]

`-DOVERLAY_CONFIG`
https://github.com/zephyrproject-rtos/zephyr/issues/52996#issuecomment-1348484066

## Kconfig hardening

[Hardening tool][zephyr-hardening]

## Overlays and build configurations


search: reduce footprint .rst


## A custom Kconfig symbol

TODO: our own little Kconfig option "fancy greeting", and then require CONFIG_PRINTK or CONFIG_LOG
TODO: release and debug configurations






## Logging

`printk` output is sent immediately, without any mutual exclusion or buffering.
logging: deferred. done by which thread? can we use gdb to list all threads?

build/zephyr/.config
west build --board nrf52840dk_nrf52840 -d ../build -t menuconfig

Kconfig
application inherits the board configuration file, <board_name>_defconfig, of the board








[don't][zephyr-kconfig-dont]

zephyr/cmake/modules/kconfig.cmake
zephyr/cmake/modules/configuration_files.cmake
`prj_<name>.conf pattern for auto inclusion of board overlays,

> Devicetree and Kconfig coexist and have different use. But not entirely separated ...
Kconfig
config NUM_IRQS
	default 48










[nordicsemi]: https://www.nordicsemi.com/
[nordicsemi-nrf52840-dk]: https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk
[nrf-vscode-kconfig]: https://marketplace.visualstudio.com/items?itemName=nordic-semiconductor.nrf-kconfig
[nrf-connect-sdk]: https://www.nordicsemi.com/Products/Development-software/nrf-connect-sdk
[zephyr-build]: https://docs.zephyrproject.org/latest/build/index.html
[zephyr-cmake]: https://docs.zephyrproject.org/latest/build/cmake/index.html
[zephyr-printk]: https://docs.zephyrproject.org/latest/services/logging/index.html#printk
[zephyr-logging]: https://docs.zephyrproject.org/latest/services/logging/index.html
[zephyr-kconfig]: https://docs.zephyrproject.org/latest/build/kconfig/index.html#configuration-system-kconfig
[zephyr-kconfig-merge]: https://docs.zephyrproject.org/latest/build/kconfig/setting.html#the-initial-configuration
[zephyr-kconfig-extra]: https://docs.zephyrproject.org/latest/develop/application/index.html#important-build-system-variables
[zephyr-kconfig-dont]: https://docs.zephyrproject.org/latest/build/kconfig/tips.html#what-not-to-turn-into-kconfig-options
[zephyr-kconfig-search]: https://docs.zephyrproject.org/latest/kconfig.html
[zephyr-kconfig-tips]: https://docs.zephyrproject.org/latest/build/kconfig/tips.html#kconfig-tips-and-best-practices
[zephyr-kconfig-syntax]: https://docs.zephyrproject.org/latest/build/kconfig/setting.html#setting-kconfig-configuration-values
[zephyr-hardening]: https://docs.zephyrproject.org/latest/security/hardening-tool.html
[zephyr-dts]: https://docs.zephyrproject.org/latest/build/dts/index.html
[golioth]: https://golioth.io/
[golioth-kconfig-diff]: https://blog.golioth.io/zephyr-quick-tip-show-what-menuconfig-changed-and-make-changes-persistent/
[serial-dt]: https://www.decisivetactics.com/products/serial/
[serial-putty]: https://www.putty.org/
[kernel-config]: https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html
[zephyr-dconf]: https://docs.zephyrproject.org/latest/develop/application/index.html#basics