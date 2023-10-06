
Logging as demo for Kconfig

- [Goals](#goals)
- [Prerequisites](#prerequisites)
- [Getting started using `printk`](#getting-started-using-printk)
- [Kconfig](#kconfig)
  - [Configuring symbols](#configuring-symbols)
- [Navigating Kconfig](#navigating-kconfig)
  - [Loading `menuconfig` and `guiconfig`](#loading-menuconfig-and-guiconfig)
  - [Persisting `menuconfig` and `guiconfig` changes](#persisting-menuconfig-and-guiconfig-changes)
  - [Kconfig search](#kconfig-search)
  - [Nordic's Kconfig extension for `vscode`](#nordics-kconfig-extension-for-vscode)
- [Using different configuration files](#using-different-configuration-files)
  - [Build types and alternative Kconfig files](#build-types-and-alternative-kconfig-files)
  - [Board-specific Kconfig fragments](#board-specific-kconfig-fragments)
  - [Extra configuration files](#extra-configuration-files)
- [Kconfig hardening](#kconfig-hardening)
- [A custom Kconfig symbol](#a-custom-kconfig-symbol)
  - [Adding a custom configuration](#adding-a-custom-configuration)
  - [Configuring the application build using `Kconfig`](#configuring-the-application-build-using-kconfig)
  - [Using the application's `Kconfig` symbol](#using-the-applications-kconfig-symbol)
- [Summary](#summary)
- [Further reading](#further-reading)

## Goals

In this section, we'll explore the [Kconfig configuration system][zephyr-kconfig] by looking at the `printk` logging option in Zephyr. [_Logging_][zephyr-logging] in Zephyr is everything from simple [`print`-style text logs][zephyr-printk] to customized messaging. Notice, though, that we'll _not_ explore the logging service in detail, but only use it as an excuse to dig into [Kconfig][zephyr-kconfig]. Finally, we'll create our own little application specific `Kconfig` configuration.

TODO: Kconfig == Kernel configuration.

## Prerequisites

Zephyr installed, follow previous chapter? Kconfig comes "preinstalled" as python script
zephyr/scripts/kconfig/kconfig.py

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

The simplest way to log text in Zephyr is the `printk` function. The `printk` also takes multiple arguments and format strings, but that's not what we want to show here, so we'll just modify the `main` function to output the string _"Message in a bottle."_ each time it is called.

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

The connection settings for the UART interface are configured using the [Devicetree][zephyr-dts]. We'll explore the devicetree in a later chapter. Since these default settings are all we need for now, let's focus on Kconfig and go ahead and build and flash the project:

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

> **Notice:** The command line output of `west build` also shows the order in which `Kconfig` merges the configuration files. We'll see this in the section about [build types and alternative Kconfig files](#build-types-and-alternative-kconfig-files).

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

While this search does point out that there is a dependency to the `BOOT_BANNER` symbol, it cannot know our current configuration and therefore can't tell us that we can't only disable `PRINTK` since `BOOT_BANNER` is also enabled in our configuration.


### Nordic's Kconfig extension for `vscode`

Another graphical user interface for `Kconfig` is the [nRF Kconfig][nrf-vscode-kconfig] extension for `vscode`. This is an extension tailored for use with the [nRF Connect SDK][nrf-connect-sdk], but to some extent this extension can also be used for a generic Zephyr project and can therefore also be useful if you're not developing for a target from [Nordic][nordicsemi]:

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

However, as visible in the above screenshot, at the time of writing the extension fails to inform the user about the dependency to the `BOOT_BANNER` and in contrast to `menuconfig` and `guiconfig` it is therefore not visible why the configuration option cannot be disabled. Aside from that, this extension has two major benefits:

* The _Save minimal config_ option seems to recognize options coming from a `_defconfig` file and therefore really only exports the configuration options set within the extension.
* The extension adds auto-completion to your `.conf` files.

With this last tool to explore `Kconfig`, let's have a look at a couple of more well used options.



## Using different configuration files

Until now we've only used the _application configuration file_ [prj.conf](../00_empty/prj.conf) for setting `Kconfig` symbols. In addition to this configuration file, the Zephyr build system automatically picks up additional `Kconfig` _fragments_, if provided, and also allows explicitly specifying additional or alternative fragments.

We'll quickly glance through the most common practices here.

### Build types and alternative Kconfig files

Zephyr doesn't use pre-defined build types such as _Debug_ or _Release_ builds. It is up to the application to decide the optimization options and build types. Different build types in Zephyr are supported by [alternative Kconfig files][zephyr-dconf], specified using the [`CONF_FILE`][zephyr-kconfig-extra] build system variable.

Let's freshen up on the _build system variables_: These variables are _not_ direct parameters for the build and configuration systems used by Zephyr, e.g., like the `--build-dir` option for `west build`, but are available globally. As described in the [official documentation on important build system variables][zephyr-kconfig-extra], there are multiple ways to pass such variables to the build system. We'll repeat two of the available options here:

1. A build system variable may exist as environment variable.
2. A build system variable may be passed to `west build` as [one-time CMake arguments][zephyr-west-dashdash] after a double dash `--`.

We'll stick to the second option and use a [one-time CMake argument][zephyr-west-dashdash], as shown in the [application development basics in the official documentation][zephyr-dconf]. First, let's go ahead and create a [`prj_release.conf`](./prj_release.conf), which defines our symbols for the `release` build type:

```
$ tree --charset=utf-8 --dirsfirst
.
├── src
│   └── main.c
├── CMakeLists.txt
├── prj.conf
└── prj_release.conf

$ cat prj_release.conf
```
```conf
CONFIG_LOG=n
CONFIG_PRINTK=n
CONFIG_BOOT_BANNER=n
CONFIG_EARLY_CONSOLE=n
CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT=y
CONFIG_USE_SEGGER_RTT=n
CONFIG_BUILD_OUTPUT_STRIPPED=y
CONFIG_FAULT_DUMP=0
CONFIG_STACK_SENTINEL=y
```

For now, don't worry about the symbols in this file, we'll catch up on them in the section about [Kconfig hardening](#kconfig-hardening). We can now instruct `west` to use this `prj_release.conf` _instead_ of our `prj.conf` for the build by using the following command:

```bash
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 -d ../build -- -DCONF_FILE=prj_release.conf
```

If you scroll through the output of the `west build` command, you'll notice that `Kconfig` will now merge `prj_release.conf` into our final configuration:

TODO: its actually a python script invoked via the CMake extension, show.

```
Parsing /opt/nordic/ncs/v2.4.0/zephyr/Kconfig
Loaded configuration '/opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig'
Merged configuration '/path/to/01_kconfig/prj_release.conf'
Configuration saved to '/path/to/zephyr_practical/build/zephyr/.config'
```

Thus, in short, Zephyr accepts build types by specifying an alternative `Kconfig` file with a file name format `prj_<build>.conf` using the `CONF_FILE` build system variable. The name of the build type is entirely application specific and does not imply any, e.g., compiler optimizations.

> **Notice:** Zephyr uses `Kconfig` to specify build types and also optimizations. Thus, `CMake` options such as [`CMAKE_BUILD_TYPE`][cmake-build-type] are typically **not** used directly in Zephyr projects.

### Board-specific Kconfig fragments

Aside from the application configuration file [prj.conf](../00_empty/prj.conf), Zephyr also automatically picks up board specific `Kconfig` fragments. Such fragments are placed in the `boards` directory in the project root (next to the `CMakeLists.txt` file) and use the `<board>.conf` name format.

E.g., throughout this guide we're using the nRF52840 development kit which has the board name `nrf52840dk_nrf52840`. Thus, the file `boards/nrf52840dk_nrf52840.conf` will automatically merged into the `Kconfig` configuration during the build.

Let's try this out by disabling UART as console output using a new file `boards/nrf52840dk_nrf52840.conf`. For the nRF52840 development kit this symbol is enabled by default, you can go ahead and verify this by checking the default configuration `zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig` - or take my word for it:

```bash
tree --charset=utf-8 --dirsfirst
.
├── boards
│   └── nrf52840dk_nrf52840.conf
├── src
│   └── main.c
├── CMakeLists.txt
├── prj.conf
└── prj_release.conf

$ cat boards/nrf52840dk_nrf52840.conf
```
```conf
CONFIG_UART_CONSOLE=n
```

Then, we perform a _pristine_ build of the project.


```bash
$ west build --board nrf52840dk_nrf52840 -d ../build --pristine
```

In the output of `west build` you should see that the new file `boards/nrf52840dk_nrf52840.conf` is indeed merged into the `Kconfig` configuration. In fact, you should also see a warning that the Zephyr library `drivers__console` is excluded from the build since it now has no `SOURCES`:

```
Parsing /opt/nordic/ncs/v2.4.0/zephyr/Kconfig
Loaded configuration '/opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig'
Merged configuration '/path/to/01_kconfig/prj.conf'
Merged configuration '/path/to/01_kconfig/boards/nrf52840dk_nrf52840.conf'
Configuration saved to '/path/to/build/zephyr/.config'
...
CMake Warning at /opt/nordic/ncs/v2.4.0/zephyr/CMakeLists.txt:838 (message):
  No SOURCES given to Zephyr library: drivers__console
  Excluding target from build.
```

You can of course verify that the symbol is indeed disabled by checking the `zephyr/.config` file in the `build` directory.

> **Notice:** Previous versions of Zephyr used a `prj_<board>.conf` board in the project root directory and at the time of writing the build system will still merge any such file into the configuration. This file, however, is deprecated and the `boards/<board>.conf` should be used instead.

But how does this work with our previously created `release` build type? Let's try it out and have a look at the build output:

```bash
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 -d ../build -- -DCONF_FILE=prj_release.conf
```
```
Parsing /opt/nordic/ncs/v2.4.0/zephyr/Kconfig
Loaded configuration '/opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig'
Merged configuration '/path/to/01_kconfig/prj_release.conf'
Configuration saved to '/path/to/build/zephyr/.config'
```

Well, this is awkward: Even though we provided a board specific `Kconfig` fragment it is ignored as soon as we switch to our alternative `prj_release.conf` configuration. This is, however, [working as intended][zephyr-kconfig-merge]: When using alternative `Kconfig` files in the format `prj_<build>.conf`, Zephyr looks for a board fragment matching the file name `boards/<board>_<build>.conf`.

In our case, we can create the file `boards/nrf52840dk_nrf52840_release.conf`. Let's get rid of the warning for the Zephyr library `drivers__console` by disabling the `CONSOLE` entirely in our board release configuration:

```bash
tree --charset=utf-8 --dirsfirst
.
├── boards
│   ├── nrf52840dk_nrf52840.conf
│   └── nrf52840dk_nrf52840_release.conf
├── src
│   └── main.c
├── CMakeLists.txt
├── prj.conf
└── prj_release.conf

$ cat boards/nrf52840dk_nrf52840_release.conf
```
```conf
CONFIG_CONSOLE=n
CONFIG_UART_CONSOLE=n
```

Now, when we rebuild the project we'll see that our board fragment is merged into the `Kconfig` configuration, and no more warning should be thrown by `west build`:

```bash
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 -d ../build -- -DCONF_FILE=prj_release.conf
```
```
Parsing /opt/nordic/ncs/v2.4.0/zephyr/Kconfig
Loaded configuration '/opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig'
Merged configuration '/path/to/01_kconfig/prj_release.conf'
Merged configuration '/path/to/01_kconfig/boards/nrf52840dk_nrf52840_release.conf'
Configuration saved to '/path/to/build/zephyr/.config'
```

### Extra configuration files

There is one more option to modify the `Kconfig`, listed in the [important build system variables][zephyr-kconfig-extra]: `EXTRA_CONF_FILE`. This build system variable accepts one or more additional `Kconfig` fragments. This option can be useful, e.g., to specify additional configuration options used by multiple build types (normal builds, "release" builds, "debug" builds) in a separate fragment.

> Since [Zephyr 3.4.0][zephyr-rn-3.4.0] the `EXTRA_CONF_FILE` build system variable replaces the deprecated variable `OVERLAY_CONFIG`.

Let's add two `Kconfig` fragments `extra0.conf` and `extra1.conf`:

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── boards
│   ├── nrf52840dk_nrf52840.conf
│   └── nrf52840dk_nrf52840_release.conf
├── src
│   └── main.c
├── CMakeLists.txt
├── extra0.conf
├── extra1.conf
├── prj.conf
└── prj_release.conf

$ cat extra0.conf
CONFIG_DEBUG=n

$ cat extra1.conf
CONFIG_GPIO=n
```

We can now pass the two extra configuration fragments to the build system using the `EXTRA_CONF_FILE` variable. The paths are relative to the project root and can either be separated using semicolons or spaces:

```bash
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 -d ../build -- \
  -DEXTRA_CONF_FILE="extra0.conf;extra1.conf"
```
```
Parsing /opt/nordic/ncs/v2.4.0/zephyr/Kconfig
Loaded configuration '/opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig'
Merged configuration '/path/to/01_kconfig/prj.conf'
Merged configuration '/path/to/01_kconfig/boards/nrf52840dk_nrf52840.conf'
Merged configuration '/path/to/01_kconfig/extra0.conf'
Merged configuration '/path/to/01_kconfig/extra1.conf'
Configuration saved to '/path/to/build/zephyr/.config'
```

The fragments specified with the `EXTRA_CONF_FILE` variable are merged into the final configuration in the given order. E.g., let's create a `release` build and reverse the order of the fragments:

```bash
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 -d ../build -- \
  -DCONF_FILE="prj_release.conf" \
  -DEXTRA_CONF_FILE="extra1.conf;extra0.conf"
```
```
Parsing /opt/nordic/ncs/v2.4.0/zephyr/Kconfig
Loaded configuration '/opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig'
Merged configuration '/path/to/01_kconfig/prj_release.conf'
Merged configuration '/path/to/01_kconfig/boards/nrf52840dk_nrf52840_release.conf'
Merged configuration '/path/to/01_kconfig/extra1.conf'
Merged configuration '/path/to/01_kconfig/extra0.conf'
Configuration saved to '/path/to/build/zephyr/.config'
```



## Kconfig hardening

One more tool worth mentioning when talking about `Kconfig` is the [hardening tool][zephyr-hardening]. Just like `menuconfig` and `guiconfig`, this tool is a target for `west build`. It is executed using the `-t hardenconfig` target in the call to `west build`. The hardening tool checks the `Kconfig` symbols against a set of known configuration options that should be used for a secure Zephyr application, and lists all differences found in the application.

We can use this tool to check our normal and `release` configurations:

```bash
$ # test normal build
$ west build --board nrf52840dk_nrf52840 \
  -d ../build \
  --pristine \
  -t hardenconfig

$ # test release build
$ west build --board nrf52840dk_nrf52840 \
  -d ../build \
  --pristine \
  -t hardenconfig \
  -- -DCONF_FILE=prj_release.conf
```

For the normal build using `prj.conf`, the hardening tool will display a table of all symbols whose current value does not match the recommended, secure configuration value, e.g., at the time of writing this is the output for the normal build:

```
-- west build: running target hardenconfig
[0/1] cd /path/to/zephyr/kconfig/hardenconfig.py /opt/nordic/ncs/v2.4.0/zephyr/Kconfig
                 name                 | current | recommended || check result
==============================================================================
CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT |    n    |      y      ||     FAIL
CONFIG_USE_SEGGER_RTT                 |    y    |      n      ||     FAIL
CONFIG_BUILD_OUTPUT_STRIPPED          |    n    |      y      ||     FAIL
CONFIG_FAULT_DUMP                     |    2    |      0      ||     FAIL
CONFIG_STACK_SENTINEL                 |    n    |      y      ||     FAIL
```

The symbols in `prj_release.conf` have been chosen such that at the time of writing all hardening options are fulfilled and the above table is empty.



## A custom Kconfig symbol

In the previous sections, we had a thorough look at `Kconfig` and its workings. To wrap up this chapter, we'll create a custom, application specific symbol and use it our application and build process. While this is a rather unlikely use case, it nicely demonstrates how `Kconfig` works.

> This section borrows from the [nRF Connect SDK Fundamentals lesson on configuration files][nordicsemi-academy-kconfig] that is freely available in the [Nordic Developer Academy][nordicsemi-academy]. If you're looking for additional challenges, check out the available courses!

### Adding a custom configuration

[CMake automatically detects a `Kconfig` file][zephyr-kconfig-cmake] if it is placed in the same directory of the application's `CMakeLists.txt`, and that is what we'll use to for our own configuration file. In case you want to place the `Kconfig` file somewhere else, you can customize this behavior using an absolute path for the `KCONFIG_ROOT` build system variable.

Similar to the file `zephyr/subsys/debug/Kconfig` of Zephyr's _debug_ subystem, the `Kconfig` file specifies all available configuration options and their dependencies. We'll only specify a simple boolean symbol in the `Kconfig` file in our application's root directory. Please refer to the [official documentation][zephyr-kconfig-syntax] for details about the `Kconfig` syntax:


```bash
$ tree --charset=utf-8 --dirsfirst -L 1
.
├── boards
├── src
├── CMakeLists.txt
├── Kconfig
├── extra0.conf
├── extra1.conf
├── prj.conf
└── prj_release.conf

$ cat Kconfig
```
```conf
mainmenu "Customized Menu Name"
source "Kconfig.zephyr"

menu "Application Options"
config USR_FUN
	bool "Enable usr_fun"
	default n
	help
	  Enables the usr_fun function.
endmenu
```

The skeleton of this `Kconfig` file is well explained in the [official documentation][zephyr-kconfig-cmake]:

The statement _"mainmenu"_ simply defines some text that is used as our `Kconfig` main menu. It is shown, e.g., as title in the build target `menuconfig`. We'll see this in a moment when we'll build our target and launch `menuconfig`.

The _"source"_ statement essentially includes the top-level Zephyr Kconfig file `zephyr/Kconfig.zephyr` and all of its symbols (all _"source"_ statements are relative to the Zephyr root directory). This is necessary since we're effectively replacing the `zephyr/Kconfig` file of the Zephyr base that is usually _parsed_ as the first file by `Kconfig`. We'll see this below when we look at the build output. The contents of the default root `Kconfig` file are quite similar to what we're doing right now:

```bash
$ cat $ZEPHYR_BASE/Kconfig
# -- hidden comments --
mainmenu "Zephyr Kernel Configuration"
source "Kconfig.zephyr"
```

Thus, by sourcing the `Kconfig.zephr` file, we're loading all `Kconfig` menus and symbols provided with Zephr. Next, we declare our own menu between the _"menu"_ and _"endmenu"_ statements to group our application symbols. Within this menu, we declare our `USR_FUN` symbol, which we'll use to enable a function `usr_fun`.

Let's rebuild our application without configuring `USR_FUN` and have a look at the build output:

```bash
$ west build --board nrf52840dk_nrf52840 -d ../build --pristine
```
```
Parsing /path/to/01_kconfig/Kconfig
Loaded configuration '/opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840_defconfig'
Merged configuration '/path/to/01_kconfig/prj.conf'
Merged configuration '/path/to/01_kconfig/boards/nrf52840dk_nrf52840.conf'
Configuration saved to '/path/to/build/zephyr/.config'
Kconfig
```

Have a good look at the very first line of the `Kconfig` related output:
* In our previous builds, this line indicated the use of Zephyr's default `Kconfig` file as follows:
  `Parsing /opt/nordic/ncs/v2.4.0/zephyr/Kconfig`,
* Whereas now it uses our newly created `Kconfig` file:
  `Parsing /path/to/01_kconfig/Kconfig`.

The remainder of the output should look familiar. When we look at the generated `zephyr/.config` file in our build directory, we'll see that the `USR_FUN` symbol is indeed set according to its default value `n`:

```bash
$ cat ../build/zephyr/.config
```
```conf
# Application Options
#
# CONFIG_USR_FUN is not set
# end of Application Options
```

The new, custom main menu name _"Customized Menu Name"_ is only visible in the interactive `Kconfig` tools, such as `menuconfig`, and is not used within the generated `zephyr/.config` file. The interactive tools now also show our newly created menu and symbol.

As you can see in the below screenshot, the main menu has the name _"Customized Menu Name"_ and a new menu _"Application Options"_ is now available as the last entry of options:

```bash
$ west build -d ../build -t menuconfig
```

![Screenshot menuconfig with custom mainmenu](../assets/kconfig-custom.png?raw=true "menuconfig")
![Screenshot menuconfig with application menu](../assets/kconfig-application.png?raw=true "menuconfig")

> **Notice:** It is also possible to source `Kconfig.zephyr` _after_ defining the application symbols and menus. This would have the effect that your options will be listed _before_ the Zephyr symbols and menus. In this section, we've sourced `Kconfig.zephr` before our own options since this is also the case in the template used by the official documentation.

### Configuring the application build using `Kconfig`

Now that we have our own `Kconfig` symbol, we can make use of it in the application's build process. Create two new files `usr_fun.c` and `usr_fun.h` in the `src` directory:

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── boards
│   ├── nrf52840dk_nrf52840.conf
│   └── nrf52840dk_nrf52840_release.conf
├── src
│   ├── main.c
│   ├── usr_fun.c
│   └── usr_fun.h
├── CMakeLists.txt
├── Kconfig
├── extra0.conf
├── extra1.conf
├── prj.conf
└── prj_release.conf
```

We're defining a simple function `usr_fun` that prints an additional message on boot using the newly created files:

```c
// Content of usr_fun.h
#pragma once

void usr_fun(void);
```
```c
// Content of usr_fun.c
#include "usr_fun.h"
#include <zephyr/kernel.h>

void usr_fun(void)
{
    printk("Message in a user function.\n");
}
```

Now all that's left to do is to tell `CMake` about our new files. Instead of always including these files when building our application, we use our `Kconfig` symbol `USR_FUN` to only include the files in the build in case the symbol is enabled by adding the following to our `CMakeLists.txt`:

```cmake
target_sources_ifdef(
    CONFIG_USR_FUN
    app
    PRIVATE
    src/usr_fun.c
)
```

`target_sources_ifdef` is another `CMake` extension from `zephyr/cmake/modules/extensions.cmake`: It conditionally includes our files in the build in case `CONFIG_USR_FUN` is defined. Since the symbol `USR_FUN` is _disabled_ by default, our build currently does not include `usr_fun`.


### Using the application's `Kconfig` symbol

Let's first extend our `main.c` file to use `usr_fun` as if it were a regular application:

```c
#include <zephyr/kernel.h>
#include "usr_fun.h"

#define SLEEP_TIME_MS 100U

void main(void)
{
    printk("Message in a bottle.\n");
    usr_fun();

    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
```

Unsurprisingly, building this application fails since `usr_fun.c` is not included in the build:

```bash
$ west build --board nrf52840dk_nrf52840 -d ../build --pristine
```
```
[158/168] Linking C executable zephyr/zephyr_pre0.elf
FAILED: zephyr/zephyr_pre0.elf zephyr/zephyr_pre0.map
--snip--
/path/to/ld.bfd: app/libapp.a(main.c.obj): in function `main':
/path/to/main.c:9: undefined reference to `usr_fun'
collect2: error: ld returned 1 exit status
ninja: build stopped: subcommand failed.
FATAL ERROR: command exited with status 1: /path/to/cmake --build /path/to/build
```

We have two possibilities to fix our build: The first possibility is to enable `USR_FUN` in our kernel configuration, e.g., in our `prj.conf` file:

```conf
# --snip--
CONFIG_USR_FUN=y
```

This, however, kind of defeats the purpose of having a configurable build: The build would still fail in case we're not enabling `USR_FUN`. Instead, we can also use the `USR_FUN` symbol within our code to only conditionally include parts of our application:

```c
#include <zephyr/kernel.h>

#ifdef CONFIG_USR_FUN
#include "usr_fun.h"
#endif

#define SLEEP_TIME_MS 100U

void main(void)
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
```

Now our build succeeds for either configuration of the `USR_FUN` symbol. Clearly, we could also use an approach similar to `printk` and provide an empty function or definition for `usr_fun` instead of adding conditional statements to our application.

> **Note:** The application builds and links, but you won't see any output on the serial console in case you've followed along: The `Kconfig` symbols responsible for the serial output are still disabled within `prj.conf` and the board specific fragment. Feel free to update your configuration, e.g., by enabling the debug output for normal builds while disabling it for the "release" build!

To understand why we can use the `CONFIG_USR_FUN` definition we can have a look at the compiler commands used to compiler `main.c`. Conveniently (as mentioned by the previous chapter), by default Zephyr enables the `CMake` variable [`CMAKE_EXPORT_COMPILE_COMMANDS`][cmake-compile-commands]. The compiler command for `main.c` is thus captured by the `compile_commands.json` in our build directory:

```json
{
  "directory": "/path/to/build",
  "command": "/path/to/bin/arm-zephyr-eabi-gcc --SNIP-- -o CMakeFiles/app.dir/src/main.c.obj -c /path/to/main.c",
  "file": "/path/to/main.c"
},
```

Within the large list of parameters passed to the compiler, there is also the `-imacros` option specifying the `autoconf.h` Kconfig header file:

```
-imacros /path/to/build/zephyr/include/generated/autoconf.h
```

This header file contains the configured value of the `USR_FUN` symbol as macro:

```c
// --snip---
#define CONFIG_USR_FUN 1
```

Looking at the [official documentation of the `-imacros` option for `gcc`][gcc-imacros], you'll find that this option acquires all the macros of the specified header without also processing its declarations. Thus, all macros within the `autoconf.h` files are also available at compile time.



## Summary

In this chapter we've explored the _kernel configuration system_ `Kconfig` in great detail. Even if you've never used `Kconfig` before, you should now be able to at least use `Kconfig` with Zephyr, and - in case you're stuck - you should be able to easily navigate the official documentation. In this chapter w've seen:

- How to find and change existing `Kconfig` symbols,
- how to analyze dependencies between different symbols,
- files that are automatically picked up by the build system,
- files generated by `Kconfig`,
- how to define your own build types and board specific fragments,
- how to define application specific `Kconfig` symbols, and
- how the build system and the application uses `Kconfig` symbols.

Recap: Kconfig is not "manually" explored, too many files

TODO: creating your own Kconfig will be much more important in case you're building your own device drivers

## Further reading

TODO:

[don't][zephyr-kconfig-dont]

zephyr/cmake/modules/kconfig.cmake
zephyr/cmake/modules/configuration_files.cmake
`prj_<name>.conf pattern for auto inclusion of board overlays,

> Devicetree and Kconfig coexist and have different use. But not entirely separated ...
Kconfig
config NUM_IRQS
	default 48

Custom Kconfig Preprocessor Functions
https://docs.zephyrproject.org/latest/build/kconfig/preprocessor-functions.html






[nordicsemi-academy]: https://academy.nordicsemi.com/
[nordicsemi-academy-kconfig]: https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-3-elements-of-an-nrf-connect-sdk-application/topic/configuration/
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
[zephyr-kconfig-cmake]: https://docs.zephyrproject.org/latest/develop/application/index.html#application-cmakelists-txt
[zephyr-hardening]: https://docs.zephyrproject.org/latest/security/hardening-tool.html
[zephyr-dts]: https://docs.zephyrproject.org/latest/build/dts/index.html
[zephyr-west-dashdash]: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html#one-time-cmake-arguments
[zephyr-rn-3.4.0]: https://docs.zephyrproject.org/latest/releases/release-notes-3.4.html
[golioth]: https://golioth.io/
[golioth-kconfig-diff]: https://blog.golioth.io/zephyr-quick-tip-show-what-menuconfig-changed-and-make-changes-persistent/
[serial-dt]: https://www.decisivetactics.com/products/serial/
[serial-putty]: https://www.putty.org/
[kernel-config]: https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html
[zephyr-dconf]: https://docs.zephyrproject.org/latest/develop/application/index.html#basics
[cmake-build-type]: https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
[cmake-compile-commands]: https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html
[gcc-imacros]: https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html



TODO:! only if Kconfig.zephyr is sourced _after_ the application menu

-- Including generated dts.cmake file: /path/to/build/zephyr/dts.cmake

warning: PRINTK (defined at subsys/debug/Kconfig:202) was assigned the value 'n' but got the value
'y'. See http://docs.zephyrproject.org/latest/kconfig.html#CONFIG_PRINTK and/or look up PRINTK in
the menuconfig/guiconfig interface. The Application Development Primer, Setting Configuration
Values, and Kconfig - Tips and Best Practices sections of the manual might be helpful too.


TODO: autoconf.h