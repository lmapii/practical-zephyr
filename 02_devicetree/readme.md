
- [Goals](#goals)
- [Prerequisites](#prerequisites)
- [What's a `devicetree`?](#whats-a-devicetree)
- [Devicetree and Zephyr](#devicetree-and-zephyr)
  - [CMake integration](#cmake-integration)
  - [Devicetree includes and sources in Zephyr](#devicetree-includes-and-sources-in-zephyr)
  - [Compiling the devicetree](#compiling-the-devicetree)
- [Devicetree basics](#devicetree-basics)
  - [Basic source file syntax](#basic-source-file-syntax)
    - [Node names, unit-addresses, `reg` and labels](#node-names-unit-addresses-reg-and-labels)
    - [Property names and basic value types](#property-names-and-basic-value-types)
  - [References and `phandle` types](#references-and-phandle-types)
    - [`phandle`](#phandle)
    - [`path`, `phandles` and `phandle-array`](#path-phandles-and-phandle-array)
  - [Summary of property types](#summary-of-property-types)
  - [Semantics, `compatible` and bindings](#semantics-compatible-and-bindings)
  - [`aliases` and `chosen` nodes](#aliases-and-chosen-nodes)
  - [Summary of file types](#summary-of-file-types)
- [Devicetree and Zephyr, take two](#devicetree-and-zephyr-take-two)
  - [Macrobatics](#macrobatics)
  - [The `_dt` API](#the-_dt-api)
  - [pinctrl ???](#pinctrl-)
  - [Important properties such as status "okay"](#important-properties-such-as-status-okay)
- [Summary](#summary)
- [Further reading](#further-reading)

## Goals

In the previous chapter, we've had a look at how to configure _software_, and we've silently assumed that there's a UART interface on our board that is configurable and used for logging.

In this chapter, we'll see how we configure and _use_ our peripherals. For this, Zephyr borrows another tool from the Linux kernel: the **devicetree** compiler. Similar to what we've seen with `Kconfig`, the `devicetree` has been adapted to better fit the needs of Zephyr when comparing it to Linux. We'll have a short look at this as well.

We'll leverage the fact that we're using a development kit with predefined peripherals, have a look at the configuration files that exist in Zephyr, and finally we'll see how we can change them to our needs.

In case you've already had an experience with `devictree`s, a short heads-up: We will _not_ define our own, custom board, and we will _not_ describe memory layouts. This is a more advanced topic and goes beyond this guide. With this chapter, we just want to have a detailed look and familiarize with the tool and files.

TODO: History? Honorable mention of Linaro?

## Prerequisites

TODO: This is part of a series, if you did not read the previous chapters, the least you need is a running project. we're using the Nordic devkit as a base to cheat the installation efforts.

TODO: Knowledge of `Kconfig` is assumed, Zephyr installed. If not, follow along the previous two chapters.

TODO: must know how to draft a zephyr freestanding application

TODO: accompanying repository, build root outside of application root.


## What's a `devicetree`?

Let's first deal with the terminology: In simple words, the _devicetree_ is a tree data structure that you provide to describe your hardware. Each _node_ describes one _device_, e.g., the UART peripheral that we used for logging via `printk` in the previous chapter. Except for the root note, each node has exactly one parent, thus the term _devicetree_.

Devicetree files use their own _DTS (Devicetree Source) format_, defined in the [Devicetree Specification][devicetree-spec]. For certain file types used by `devicetree`, Zephyr uses yet another file format - but fear not, it simply replaces the _DTS format_ by simple `.yaml` files. There are also some more subtle differences between the official devicetree specification and the way it is used in Zephyr, but we'll touch up on that throughout this guide.

The build system takes this _devicetree_ specification and feeds it to its own compiler, which - in case of Zephyr - generates `C` macros data structures that are used by the Zephyr device drivers. For all the readers coming straight from Linux - yes, this approach is a little different than what you're used to, but we'll get to that.

The following is a snippet of Nordic's the _Device Tree Source Include_ file of their nRF52840 SoC:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
#include <arm/armv7-m.dtsi>
#include "nrf_common.dtsi"

/ {
  soc {
    uart0: uart@40002000 {
      compatible = "nordic,nrf-uarte";
      reg = <0x40002000 0x1000>;
      interrupts = <2 NRF_DEFAULT_IRQ_PRIORITY>;
      status = "disabled";
    };
  };
};
```

One could compare the devicetree to something like a `struct` in `C` or a `JSON` object. Each node (or object) lists its properties and their values and thus describe the associated device and its configuration. The above snippet should make it obvious, though, that, e.g., in contrast to the much simpler `Kconfig` files, _devicetree_ specifications are by no means self explanatory.

Personally, I felt the details of the [`devicetree` specification][devicetree-spec] or Zephyr's great [official documentation on `devicetree`][zephyr-dts] a bit overwhelming, and I could hardly keep all the information in my head, so in this guide I'm choosing a different approach:

Instead of going into detail about _DTS (Devicetree Source) format_ and schemas, we'll start with simple project, build it, and dive straight into the input and output files used or generated by the build process. Based on those files, one by one, we'll try and figure out how this whole thing works.

In case you're looking for a more detailed description of the _DTS (Devicetree Source) format_, other authors provided much more detailed descriptions - much better than anything I could possibly come up with. Here are some of my favourites:

- My obvious first choice is the [official Devicetree specification][devicetree-spec]. Just keep in mind that Zephyr made some slight adjustments.
- Second, [Zephyr's official documentation on Devicetree][zephyr-dts] is very hard to beat.
- Zephyr's official documentation also includes a more pratical information guide in the form of a [Devicetree How-Tos][zephyr-dts].
- Finally, the official Raspberry PI documentation also has a [great section about Devicetree and the DTS syntax][rpi-devicetree].



## Devicetree and Zephyr

To get started, we create a new freestanding application with the files listed below. In case you're not familiar with the required files, the installation, or the build process, have a look at the [previous](../00_empty/readme.md) [chapters](../01_kconfig/readme.md) or the official documentation. As mentioned in the [pre-requisites](#prerequisites), you should be familiar with creating, building and running a Zephr application.

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

The `prj.conf` can remain empty for now, and the `CMakeLists.txt` only includes the necessary boilerplate to create a Zephyr application. As application, we'll use the same old `main` function that outputs the string _"Message in a bottle."_ each time it is called, and thus each time the device starts.

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

As usual, we can now build this application for the development kit of choice, in my case, the [nRF52840 Development Kit from Nordic][nordicsemi].

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build
```

But wait, we didn't "do anything devicetree" yet, did we? That's right, someone else already did it for us! After our [first look into Zephyr](../00_empty/readme.md) and after [exploring Kconfig](../01_kconfig/readme.md), we're familiar with the build output of our Zephyr application, so let's have a look! You should find a similar output right at the start of your build:

```
-- Found Dtc: /opt/nordic/ncs/toolchains/4ef6631da0/bin/dtc (found suitable version "1.6.1", minimum required is "1.4.6")
-- Found BOARD.dts: /opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts
-- Generated zephyr.dts: /path/to/build/zephyr/zephyr.dts
-- Generated devicetree_generated.h: /path/to/build/zephyr/include/generated/devicetree_generated.h
-- Including generated dts.cmake file: /path/to/build/zephyr/dts.cmake
```

### CMake integration

The build output shows us that devicetree - just like Kconfig - is a central part of any application build. It is, in fact, added to your build via the CMake module `zephyr/cmake/modules/dts.cmake`. The first thing that Zephyr is looking for, is the _devicetree compiler_ `dtc` (see, `zephyr/cmake/modules/FindDtc.cmake`). Typically, this tool is in your `$PATH` after installing Zephyr or loading your Zephyr environment:

```bash
$ where dtc
/opt/nordic/ncs/toolchains/4ef6631da0/bin/dtc
```

Since we didn't specify any so called _devicetree overlay file_ (bear with me for now, we'll see how to modify our devicetree using _overlay_ files in a later section), Zephyr then looks for a devicetree source file that matches the specified _board_. In my case, that's the nRF52840 development kit, which is supported in the current Zephyr version: The board and thus its devicetree is fully described by the file `nrf52840dk_nrf52840.dts`.

> **Note:** If you'd be using a custom board that is not supported by Zephyr, you'd have to provide your own DTS file for the board you're using. We won't go into details about adding support for a custom board in this chapter, but at the end of it you should have all the knowledge to do that - or to understand any other guide showing you how to do it.

Let's not dive into the file's contents for now and instead have a look at the build. Somehow, the build ends up with a generated `zephyr.dts` file. A couple of interesting things are happening in this process that are worth mentioning, so we'll have a closer look.

### Devicetree includes and sources in Zephyr

If you're familiar with the [devicetree specification][devicetree-spec], you might have wondered why the devicetree snippet that we've seen before uses C/C++ style `#include` directives:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```c
#include <arm/armv7-m.dtsi>
#include "nrf_common.dtsi"
```

The [devicetree specification][devicetree-spec] introduces a dedicated `/include/` compiler directive to include other sources in a DTS file. E.g., our include for `nrf_common.dtsi` should have the following format:

```dts
/include/ "nrf_common.dtsi"
```

The reason for this discrepancy is that Zephyr uses the C/C++ preprocessor to resolve includes - and for resolving actual C macros that are used within DTS files. This happens in the call to the CMake function `zephyr_dt_preprocess` in the devicetree CMake module.

Let's have a look at the include tree of the devicetree source file of the nRF52840 development kit used in my build:

```
nrf52840dk_nrf52840.dts
├── nordic/nrf52840_qiaa.dtsi
│   ├── mem.h
│   └── nordic/nrf52840.dtsi
│       ├── arm/armv7-m.dtsi
│       │   └── skeleton.dtsi
│       └── nrf_common.dtsi
│           ├── zephyr/dt-bindings/adc/adc.h
│           ├── zephyr/dt-bindings/adc/nrf-adc.h
│           │   └── zephyr/dt-bindings/dt-util.h
│           │       └── zephyr/sys/util_macro.h
│           │           └── zephyr/sys/util_internal.h
│           │               └── util_loops.h
│           ├── zephyr/dt-bindings/gpio/gpio.h
│           ├── zephyr/dt-bindings/i2c/i2c.h
│           ├── zephyr/dt-bindings/pinctrl/nrf-pinctrl.h
│           ├── zephyr/dt-bindings/pwm/pwm.h
│           ├── freq.h
│           └── arm/nordic/override.dtsi
└── nrf52840dk_nrf52840-pinctrl.dtsi
```

> **Note:** You might have noticed the file extensions `.dts` and `.dtsi`. Both file extensions are devicetree source files, but by convention the `.dtsi` extension is used for DTS files that are intended to be _included_ by other files. We'll get back to file types and extensions in a later section. TODO: ref.

It's all starting to make sense, doesn't it?
- Since we didn't specify anything external in an overlay file, our outermost devicetree source file is our _board_ `nrf52840dk_nrf52840.dts`.
- Our board uses the nRF52840 QIAA microcontroller from Nordic, which is described in its own devicetree include source file `nrf52840_qiaa.dtsi`.
- That MCU is in turn is a variant of the nRF52840, and therefore includes `nrf52840.dtsi`.
- The nRF52840 is an ARMv7 core, which is described in `armv7-m.dtsi`.
- In addition, the nRF52840 uses Nordic's peripherals, specified in `nrf_common.dtsi`.

The included C/C++ header files `.h` simply leverage the fact that Zephyr uses the preprocessor and we're therefore allowed to use macros within devicetree source files: All the macros will be replaced by their values before the actual compilation process.

This type of include graph is very common for devicetrees in Zephyr: You start with your _board_, which uses a specific _MCU_, which has a certan _architecture_ and vendor specific peripherals.

For each included file, each devicetree source file can reference or overwrite properties and nodes. E.g., for our console output we can find the following parts (the below snippets are incomplete!) in the devicetree source file of the development kit:

`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
/ {
  chosen {
    zephyr,console = &uart0;
  };
};

&uart0 {
  compatible = "nordic,nrf-uarte";
  status = "okay";
  current-speed = <115200>;
  /* other properties */
};
```

With `zephyr,console` we seem to be able to tell Zephyr that we want it to use the node `uart0` for the console output and therefore `printk` statements. We're also modifying the `&uart0` reference's properties, e.g., we set the baud rate to `115200` and enable it by setting its status to "okay".

The `uart0` node is in turn defined in the included devicetree source file of the SoC `nrf52840.dtsi`, and seems to be disabled by default.

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  soc {
    uart0: uart@40002000 {
      compatible = "nordic,nrf-uarte";
      reg = <0x40002000 0x1000>;
      interrupts = <2 NRF_DEFAULT_IRQ_PRIORITY>;
      status = "disabled";
    };
  };
};
```

But that's enough devicetree syntax for now. After this step, we end up with a single `zephyr.dts.pre` devicetree (DTS) source file.

### Compiling the devicetree

At the beginning of this chapter we mentioned that Zephyr uses the `dtc` devicetree compiler to generate the corresponding source code. That is, however, not entirely true: While `dtc` is definitely invoked during in the build process, it is not used to generate any source code. Instead, Zephyr feeds the flat `zephyr.dts.pre` into its own `GEN_DEFINES_SCRIPT` Python script, which defaults to `zephyr/scripts/dts/gen_defines.py`.

Now, why would you want to do that? The devicetree compiler `dtc` is typically used to compile devicetree sources into into a binary format called devicetree blob `dtb`. The Linux kernel, e.g., parses the DTB and uses the information to configure and initialize the hardware components described in the DTB. This allows the kernel to know how to communicate with the hardware without hardcoding this information in the kernel code. Thus, under Linux, the devicetree is parsed and loaded during _runtime_.

Zephyr, however, is designed to run on resource constrained, embedded systems. It is simply not feasible to load a devicetree during runtime: This would take up too many resources in both, the Zephyr drivers and for storing the devicetree itself. Instead, the devicetree is resolved during _compile time_.

> **Note:** In case this guide is too slow for you but you still want to know more about devicetree, there is a [brilliant video of the Zephyr Development Summit 22 by Bolivar on devicetree][zephyr-summit-22-devicetree].

So if not a binary, what's the output of this `gen_defines.py` generator? Let's have another peek at the output of our build process:

```
-- Generated zephyr.dts: /path/to/build/zephyr/zephyr.dts
-- Generated devicetree_generated.h: /path/to/build/zephyr/include/generated/devicetree_generated.h
-- Including generated dts.cmake file: /path/to/build/zephyr/dts.cmake
```

We get three files: The `zephyr.dts` that has been generated out of our preprocessed `zephyr.dts.pre`, a `devicetree_generated.h` header file, and a CMake file `dts.cmake`.

As promised, the original devicetree `dtc` compiler _is_ invoked during the build, and that's where it comes into play: The `zephyr.dts` devicetree source file is fed into `dtc`, but not to generate any binaries or source code, but to generate warnings and errors. The output itself is discarded. This helps to reduce the complexity of the Python devicetree script `gen_defines.py` and ensures that the devicetree source file used in Zephyr is at least still compatible with the original specification.

The `devicetree_generated.h` header file replaces the devicetree blob `dtb`: It is included by the drivers and our application and thereby strips all unnecessary or unused parts. **"Macrobatics"** is the term that Martì Bolivar used in his [talk about the Zephyr devicetree in the June 2022 developer summit][zephyr-summit-22-devicetree], and it fits. Even for our tiny application, the generated header is over 15000 lines of code! We'll see later how these macros are used by the Zephyr API and drivers. If you're curious, have a look at `zephyr/include/zephyr/devicetree.h`, for now, let's have a glimpse:

```c
#define DT_CHOSEN_zephyr_console DT_N_S_soc_S_uart_40002000
// --snip---
#define DT_N_S_soc_S_uart_40002000_P_current_speed 115200
#define DT_N_S_soc_S_uart_40002000_P_status "okay"
```

Looks cryptic? With just a few hints, this becomes much more readable. Know that:
- `DT_` is a common prefix for devicetree macros,
- `_S_` is simply a forward slash `/`,
- `_N_` refers to a _node_,
- `_P_` is a _property_.

Thus, e.g., `DT_N_S_soc_S_uart_40002000_P_current_speed` simply refers to the _property_ `current_speed` of the _node_ `/soc/uart_40002000`. In Zephyr, this configuration value is set during _compile time_. You'll need to recompile your application in case you want to change this property. The approach in Linux would be different: There, the (UART speed) property is read from the devicetree blob `dtb` during runtime. You could change the property, recompile the devicetree and would not need to touch your application or the Kernel.

But let's leave it at that for now, we'll have a proper look at this later. For now, it is just important to know that we'll resolve our devicetree at _compile time_ using generated macros.

Finally, the generated `dts.cmake` is a file that basically allows to access the entire devicetree also from within CMake, using CMake target properties, e.g., we'll find the _current speed_ of our UART peripheral also within CMake:

`dts.cmake`
```cmake
set_target_properties(
    devicetree_target
    PROPERTIES
    "DT_PROP|/soc/uart@40002000|current-speed" "115200"
)
```

That's it for our build. Let's wrap it up:
- Zephyr uses a so called devicetree to describe the hardware.
- Most of the devicetree sources are already available within Zephyr.
- We'll use devicetree mostly to override or extend existing DTS files.
- In Zephyr, the devicetree is resolved at _compile time_, using [macrobatics][zephyr-summit-22-devicetree].


## Devicetree basics

Now we finally take a closer look at the devicetree syntax and its files. We'll walk through it by creating a devicetree "snippet" and using a real-world reference. This section heavily borrows from existing documentation such as the [devicetree specification][devicetree-spec], [Zephyr's devicetree docs][zephyr-dts] and the [nRF Connect SDK Fundamentals lesson on devicetree][nordicsemi-academy-devicetree].

### Basic source file syntax

Let's start from scratch. We create an empty devicetree source file `.dts` with the following empty tree:
TODO: details in [DEVICETREE SOURCE (DTS) FORMAT (VERSION 1)][devicetree-spec]

```dts
/dts-v1/;
/ { /* Empty. */ };
```

TODO: C style (`/* ... \*/`) and C++ style (`//`) comments are supported.

#### Node names, unit-addresses, `reg` and labels

The first line contains the _tag_ `/dts-v1/;` identifies the file as a version _1_ devicetree source file. Without this tag, the devicetree compiler would treat the file as being of the obsolete version _0_ - which is incompatible with the current major devicetree version _1_. The tag `/dts-v1/;` is therefore required when working with Zephyr. Following the version tag is an empty devicetree: It's only _node_ is the _root node_, identified by convention by a forward slash `/`.

Within this root node, we can now define our own nodes in the form of a tree, kind of like a `JSON` object or nested `C` structure:

```dts
/dts-v1/;

/ {
  node {
    subnode {
      /* name/value properties */
    };
  };
};
```

Nodes are identified via their _node name_. Each node can have _subnodes_ and _properties_. In the above example, we have node with the name _node_, containing a subnode named _subnode_. For now, all you need to know about _properties_ is that they are name/value pairs.

A node in the devicetree can be uniquely identified by specifying the full _path_ from the root node, through all subnodes, to the desired node, separated by forward slashes. E.g., our full path to our _subnode_ is `/node/subnode`.

Node names can also have and an optional, hexadecimal _unit-address_, specified using an `@` and thus resulting in a full node name `node-name@unit-address`. E.g., we could give our `subnode` the _unit-address_ `0123ABC` as follows:

```dts
/dts-v1/;

/ {
  node {
    subnode@0123ABC {
      reg = <0x0123ABC>;
      /* properties */
    };
  };
};
```

The _unit-address_ can be used to distinguish between several subnodes of the same type. It can be a real register address, typically a base address, e.g., the base address of the register space of a specific UART interface, but also a plain instance number, you when describing a multi-core MCU by using a `/cpus` node, with two instances `cpu@0` and `cpu@1` for each CPU core.

The fact that the _unit-address_ is also used for the register address of a device is also the reason why each node with a _unit-address_ **must** have the property `reg` - and any node _without_ a _unit-address_ must _not_ have the property `reg`. While we don't know anything about the exact syntax of the property and its value yet, clearly seems redundant redundant in the above example. In a real devicetree, however, the `reg` property usually provides more information and can therefore be seen as a more detailed view of the addressable resources within a node.

Let's finish up on the node name with a convention that ensures, that each node in the devicetree can be uniquely identified by specifying its full _path_. For any node name and property at the same level in the tree:
- in the case of _node-name_ without an _unit-address_ the _node-name_ should be unique,
- or if a node has a _unit-address_, then the full `node-name@unit-address` should be unique.

Now we can address all of our nodes using their full path. As you might imagine, within a more complex devicetree the paths become quite long, and if you'd ever need to reference a node (we'll see how that works in practice later) this is quite tedious, which is why any node can be assigned a unique _node label_:

```dts
/dts-v1/;

/ {
  node {
    subnode_label: subnode@0123ABC {
      reg = <0x0123ABC>;
      /* properties */
    };
  };
};
```

Now, instead of using `/node/subnode@0123ABC` to identify a node, we can simply use the label `subnode_label` - which must be **unique** throughout the entire devicetree. We'll go into details about labels and their use in a later section, for now it is enough to know that we can use _node labels_ as a shorthand for a node's full path.

Before we have a better look at properties and thus really know why `reg` is defined the way it is, let's see how nodes look like in a real devicetree source (_include_) file in Zephyr. This is a reduced excerpt of the devicetree source of the nRF52840 microcontroller:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  soc {
    uart0: uart@40002000 {
      reg = <0x40002000 0x1000>;
    };
    uart1: uart@40028000 {
      reg = <0x40028000 0x1000>;
    };
  };
};
```

The System-On-Chip is described using the `soc` node. The `soc` node contains two _uart_ instances, which match the UARTE instances that we can find in the register map of the nRF52840 datasheet (the `E` in UARTE refers to _EasyDMA_ support):

|  ID   | Base address | Peripheral | Instance | Description               |
| :---: | :----------: | :--------: | :------: | :------------------------ |
|   0   |  0x40000000  |   CLOCK    |  CLOCK   | Clock control             |
|       |     ...      |            |          |                           |
|   2   |  0x40002000  |   UARTE    |  UARTE0  | UART with EasyDMA, unit 0 |
|   3   |  0x40003000  |    SPIM    |  SPIM0   | SPI master 0              |
|       |     ...      |            |          |                           |
|  40   |  0x40028000  |   UARTE    |  UARTE1  | UART with EasyDMA, unit 1 |
|  41   |  0x40029000  |    QSPI    |   QSPI   | External memory interface |
|       |     ...      |            |          |                           |

The _unit-address_ of each `uart` node matches its base address. The `reg` property seems to be some kind of value list enclosed in angle brackets `<...>`. The first value of the property matches the _unit-address_ and thus the base address of the UARTE instance, but the property also has a second value and thus provides more information than the _unit-address_ itself: Looking at the register map we can see that the second value `0x1000` matches the lengths of the address space that is reserved for each UARTE instance:

- The base address of the `UARTE0` instance is `0x40002000`, followed by `SPIM0` at `0x40003000`.
- The base address of the `UARTE1` instance is `0x40028000`, followed by `QSPI` at `0x40029000`.

Finally, each UART instance also has a unique label:
- `uart0` is the label of the node `/soc/uart@40002000`,
- `uart0` is the label of the node `/soc/uart@40028000`.

#### Property names and basic value types

Let's now have a look at properties. As we've already seen for the property `reg`, properties consist of a _name_ and a _value_, and are used to describe the characteristics of the node. Property names can contain:
- digits `0-9`,
- lower and uppercase letters `a-z` and `A-Z`,
- and any of the special characters `.,_+?#-`.

Most of the properties you'll encounter in Zephyr simply use `snake_case` or `kebab-case` names, though you'll also encounter properties starting with a `#`, e.g., the standard properties `#address-cells` and `#size-cells` (also described in the [devicetree specification][devicetree-spec]).

For describing property values, we'll follow the approach of [official devicetree specification (DTSpec)][devicetree-spec]. If you're not interested the specification, skip ahead to the table containing value types and matching examples. The goal of the following paragraphs is to help you understand the connection between the DTSpec and [Zephyr's documentation][zephyr-dts] - which can sometimes be confusing. Let's start with the definition of a property value from the section _"Property values"_ in the [DTSpec][devicetree-spec]:

> "A property value is an **array** of zero or more bytes that contain information associated with the property. Properties might have an empty value if conveying true-false information. In this case, the presence or absence of the property is sufficiently descriptive." [[DTSpec]][devicetree-spec]

This is basically just a fancy way of saying that devicetree supports properties without a value assignment, as well as properties with one of multiple values. The [[DTSpec]][devicetree-spec] then goes ahead and provides the following possible _"property values"_ and their representation in memory. In our table, we can skip the memory representation since we've learned that [Zephyr doesn't compile the DTS files into a binary](#compiling-the-devicetree):

| Type                   | Description                                                            |
| :--------------------- | :--------------------------------------------------------------------- |
| `<empty>`              | Empty value. `true` if the property is present, otherwise `false`.     |
| `<u32>`                | A 32-bit integer in big endian format (a "cell").                      |
| `<u64>`                | A 64-bit integer in big endian format. Consists of two `<u32>` values. |
| `<string>`             | A printable, null-terminated string.                                   |
| `<prop-encoded-array>` | An array of values, where the type is defined by the property.         |
| `<phandle>`            | A `<u32>` value used to reference another node in the devicetree.      |
| `<stringlist>`         | A list of concatenated `<string>` values.                              |

> **Note:** The term **"cell"** is typically used to refer to the individual data elements within properties and can thus be thought of as a single unit of data in a property. The [devicetree specification][devicetree-spec] v0.4 defines a **"cell"** as _"a unit of information consisting of 32 bits"_ and thus explicitly defines its size as 32 bits. Meaning it is just another name for a 32-bit integer.

If you find this information underwhelming - you're not alone. It also won't be of much help when looking at the property values from Zephyr's devicetree source files. The missing link to understanding the nature of the above table is the following: Values in Zephyr's DTS files are **represented** using the _"Devicetree Source (DTS) Format"_ version _1_, meaning they use a specific format or **syntax** to represent the above property value types. The above table, on the other hand, is a format-agnostic list of types that any specific DTS value format must translate to. E.g., a new _"Devicetree Source (DTS) Format"_ version _2_ might use a different format to represent values of the listed types.

The information that you'll most likely be interested in is part of the later section _"Devicetree Source (DTS) Format"_ in the DTSpec, specifically the subsection _"Node and property definitions"_. There, you'll find the matching syntax description for the above types - or at least the very basic information. Thankfully, Zephyr (and most likely any other ecosystem using devicetree) doesn't just verbally describe its supported types, but uses specific type names. We'll summarize the basics below. For details, refer to the [_"type"_ section in the Zephyr's documentation on devicetree bindings][zephyr-dts-bindings-types] or [Zephyr's devicetree introduction on property values][zephyr-dts-intro-property-values].

The syntax used for property _values_ is a bit peculiar. Except for `phandles`, which we'll cover separately, the following table contains all property types supported by Zephyr and their DTSpec equivalent.

| Zephyr type    | DTSpec equivalent                      | Syntax                                                                                                  | Example                                            |
| :------------- | :------------------------------------- | :------------------------------------------------------------------------------------------------------ | :------------------------------------------------- |
| `boolean`      | `<empty>`                              | no value; a property is `true` if the property exists                                                   | `interrupt-controller;`                            |
| `string`       | `<string>`                             | double-quoted text (null terminated string)                                                             | `status = "disabled";`                             |
| `array`        | `<prop-encoded-array>`                 | 32-bit values enclosed in `<` and `>`, separated by spaces                                              | `reg = <0x40002000 0x1000>;`                       |
| `int`          | `<u32>`                                | a single 32-bit value ("cell"), enclosed in `<` and `>`                                                 | `current-speed = <115200>;`                        |
| `array`        | `<u64>`                                | 64-bit values are represented by an array of two *cells*                                                | `value = <0xBAADF00D 0xDEADBEEF>;`                 |
| `uint8-array`  | `<prop-encoded-array>` or "bytestring" | 8-bit hexadecimal values _without_ `0x` prefix, enclosed in `[` and `]`, separated by spaces (optional) | `mac-address = [ DE AD BE EF 12 34 ];`             |
| `string-array` | `<stringlist>`                         | `string`s, separated by commas                                                                          | `compatible = "nordic,nrf-egu", "nordic,nrf-swi";` |
| `compound`     | "comma-separated components"           | comma-separated values                                                                                  | `foo = <1 2>, [3, 4], "five"`                      |

The table deserves some observations and explanations:

TODO: use the same format as rpi? looks nicer ...

- A `boolean` property is `true` if the property exists, otherwise `false`.
- TODO: `string`
- `arrays` could contain elements of any supported type. In practice and in Zephyr, however, an `array` is essentially always a list of 32-bit integers: Just like in `C/C++`, the prefix `0x` is used for hexadecimals, and numbers _without_ a prefix are decimals.


<!-- | `array` | `<prop-encoded-array>` | _values_ of a property-defined type, enclosed in `<` and `>`, separated by spaces | `gpios = <&gpio0 29 0>;` | -->
<!-- TODO: wrong, not accepted by Zephyr. The `&gpio` in the example is a _reference_. _References_ are replaced by the referenced node's `phandle` - a fancy word for a 32-bit unique number - and thus still count as integer. We'll see the details a bit later. -->

TODO: an array can also be specified using `<>,<>,<>` and that is actually used for phandle arrays. It simply allows to group values, but has no effect.

- An `int`eger is represented as an `array` containing a single 32-bit value ("cell").
- Just like specified for the type `<u64>` in the [DTSpec][devicetree-spec], 64-bit integers do not have their own type, but are instead represented by an array of two `int`egers.
- In contrast to an `array`, a `uint8-array` _always_ uses **hexadecimal** literals _without_ the prefix `0x` - which can be confusing at first. E.g., `<11 12>` represents the two _32-bit_ integers with the decimal values `11` and `12`, whereas `[11, 12]` represents the two _8-bit_ integers with the decimal values `17` and `18`.
- The spaces between each byte in a `uint8-array` are optional. E.g., `mac-address = [ DEADBEEF1234 ];` is equivalent to `mac-address = [ DE AD BE EF 12 34 ];`. It practice, you'll rarely see `uint8-array`s without spaces between each byte.
- A `string-array` is just a list of strings. The only thing worth mentioning is, that the value of a `string-array` property may also be a single `string`. E.g., `compatible = "something"` is still a valid value assignment for a `string-array`.

The `compound` type is essentially a "catch-all" for custom types. Zephyr does **not** generate any macros for `compound` properties. Also, notice how compound value definitions are syntactically ambiguous, e.g., for `foo = <1 2>, "three", "four"`: Is `"three", "four"` a single value of type `string-array`, or two separate `string` values?

When reading Zephyr DTS files, keep in mind that [in Zephyr all DTS files are fed into the preprocessor](#devicetree-includes-and-sources-in-zephyr) and therefore Zephyr allows using macros in DTS files. E.g., you might encounter properties like `max-frequency = <DT_FREQ_M(8)>;`, which do not match the devicetree syntax at all. There, the preprocessor replaces the macro `DT_FREQ_M` with the corresponding literal before the source file is parsed.

The following is a snippet of a test file of Zephyr's Python devicetree generator. It contains the node `props` that nicely demonstrates the different property types supported by Zephyr.

`zephyr/scripts/dts/python-devicetree/tests/test.dts`
```dts
/dts-v1/;

/ {
  props {
    existent-boolean;
    int = <1>;
    array = <1 2 3>;
    uint8-array = [ 12 34 ];
    string = "foo";
    string-array = "foo", "bar", "baz";
  };
};
```

There's one more thing that is worth mentioning: Parentheses, arithmetic operators, and bitwise operators are allowed in property values, though the entire expression must be parenthesized. [Zephyr's introduction on property values][zephyr-dts-intro-property-values] provides the following example for a property `bar`:

```dts
/ {
  foo {
    bar = <(2 * (1 << 5))>;
  };
};
```

The property `bar` contains a single 32-bit value ("cell") with the value _64_. Notice that operators not just allowed in Zephr, but also according to the [devicetree specification][devicetree-spec]: You don't _need_ to use macros for simple arithmetic or bit operations operations.

### References and `phandle` types

We've already seen how we can create _node labels_ as shorthand form of a node's full path, but haven't really seen how such labels are used within the devicetree. Tired of all the theory? I thought so. Now that we're familiar with a good part of the devicetree source file syntax, it is time for some hands-on, so let's dive back into the command line.

We'll practice using a devicetree _overlay_ file. In a later section, we'll go into more details about what an overlay is. For now, it is enough to know that an overlay file is simply an additional DTS file on top of the hierarchy of files that is included starting with the board's devicetree source file. We can [specify an extra devicetree overlay file using the CMake variable `EXTRA_DTC_OVERLAY_FILE`][zephyr-dts-overlays], and we'll use a newly created `props-phandles.overlay` file for that:

```bash
$ mkdir -p dts/playground
$ touch dts/playground/props-phandles.overlay
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 --build-dir ../build -- \
  -DEXTRA_DTC_OVERLAY_FILE=dts/playground/props-phandles.overlay
```

The build system's output now announces that it encountered the newly created overlay file with the message `Found devicetree overlay`:

```
-- Found Dtc: /opt/nordic/ncs/toolchains/4ef6631da0/bin/dtc (found suitable version "1.6.1", minimum required is "1.4.6")
-- Found BOARD.dts: /opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts
-- Found devicetree overlay: playground/test.overlay
-- Generated zephyr.dts: /path/to/build/zephyr/zephyr.dts
```

#### `phandle`

So what is a `phandle`? If we're being picky about the terminology - it's complicated. Why?

- In the [DTSpec][devicetree-spec], we've seen that `<phandle>` is a base type.
- In addition, the [DTSpec][devicetree-spec] also defines a standard _property_ named `phandle` - ironically of type `<u32>`, and not `<phandle>`.
- In Zephyr, the term `phandle` is used pretty much only for node references in any format.

Why this ambiguity? Because in the end, any reference to a node is replaced by a unique, 32-bit value that identifies the node - the value stored in the node's `phandle` property. The fact that `phandle` _property_ is not intended to be set manually, but is instead created by the devicetree compiler for each referenced node, makes mentioning the `phandle` property as such unnecessary. Thus, the approach chosen in Zephyr's documentation - refering to any reference as `phandle` - makes a lot of sense.

But enough nit-picking, let's how this looks in a real devicetree source file. Let's create two nodes `node_a` and `node_refs` in our overlay file, and have `node_refs` reference the `node_a` once by its path and once by a label `label_a` that we create for `node_a`. How do we do this? The syntax is specified in the [DTSpec][devicetree-spec] as follows:

> "Labels are created by appending a colon ('`:`') to the label name. References are created by prefixing the label name with an ampersand ('`&`'), or they may be followed by a node's full path in braces." [DTSpec][devicetree-spec]

Thus, our devicetree overlay file, we can create the properties as follows. Notice how we're missing the DTS version `/dts-v1/;`: The version is only defined devicetree *source* files, but **not** in *overlays*. files.

```dts
/ {
  label_a: node_a { /* Empty. */ };
  node_refs {
    phandle-by-path = <&{/node_a}>;
    phandle-by-label = <&label_a>;
  };
};
```

Let's run a build to ensure that our overlay is sent through the preprocessor and the `GEN_DEFINES_SCRIPT` DTS Python script so that we can have a look at the generated `zephyr.dts`:

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build -- \
  -DEXTRA_DTC_OVERLAY_FILE=dts/playground/props-phandles.overlay
```

`build/zephyr/zephyr.dts`
```dts
/dts-v1/;

/ {
  /* Possibly lots of other nodes ... */
  label_a: node_a {
    phandle = < 0x1c >;
  };
  node_refs {
    phandle-by-path = < &{/node_a} >;
    phandle-by-label = < &label_a >;
  };
};
```

We can now see that the generator has created a `phandle` property for our referenced `node_a`. Your milage may vary on the exact value for the property since it depends on to the number of referenced nodes in the DTS file of your board. What is this `phandle` property? The [DTSpec][devicetree-spec] defines `phandle` as follows:

> Property name `phandle`, value type `<u32>`.
>
> The `phandle` property specifies a numerical identifier for a node that is unique within the devicetree. The `phandle` property value is used by other nodes that need to refer to the node associated with the property.
>
> **Note:** Most devicetrees [...] will not contain explicit phandle properties. The DTC tool automatically inserts the phandle properties when the DTS is compiled [...].

This is also what we see in the generated `zephyr.dts`: Since `node_a` is referenced by `node_ref`, Zephyr's DTS generator has inserted the property `phandle` for `node_a` in our devicetree. To what happends to the reference within this `phandle` property, we need to jump back to the syntax chapter in the [DTSpec][devicetree-spec], where we find the following:

> "In a cell array, a reference to another node will be expanded to that node's phandle."

This means that the references `&{/node_a}` and `&label_a` in our properties `phandle-by-path` and `phandle-by-label` are essentially expanded to `node_a`'s `phandle` _0x1c_. Thus, **the reference is equivalent to its phandle**. Zephyr's documentation right to refer to `&{/node_a}` and `&label_a` as "`phandle`s". Could we also define `node_a`'s `phandle` property by ourselves? Let's find out:

```dts
/ {
  label_a: node_a {
    phandle = <0xC0FFEE>;
  };
  node_refs {
    phandle-by-path = &{/node_a};
    phandle-by-label = &label_a;
  };
};
```

After executing `west build` with the previous parameters again, we indeed end up with the value `0xC0FFEE` for `node_a`'s `phandle` property:

`build/zephyr/zephyr.dts`
```dts
/dts-v1/;

/ {
  /* Possibly lots of other nodes ... */
  label_a: node_a {
    phandle = < 0xc0ffee >;
  };
  node_refs {
    phandle-by-path = < &{/node_a} >;
    phandle-by-label = < &label_a >;
  };
};
```

A single `phandle` can be useful where a 1:1 relation between nodes in the devicetree are required. E.g., the following is a snippet from the nRF52840 DTS include file, which contains a software PWM node with a configurable generator, which by default referes to `timer2`:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  /* ... */
  sw_pwm: sw-pwm {
    compatible = "nordic,nrf-sw-pwm";
    status = "disabled";
    generator = <&timer2>;
    clock-prescaler = <0>;
    #pwm-cells = <3>;
  };
};
```

#### `path`, `phandles` and `phandle-array`

We've seen the type of the _phandle_ property, but what about the types of the properties `phandle-by-path` and `phandle-by-label`? Knowing that a `phandle` is expanded to a *cell* we might guess that `phandle-by-path` and `phandle-by-label` are of type `int` or `array`: Assuming that `node_a`'s _phandle_ has the value 0xc0ffee, both `<&{/node_a}>` and `<&label_a>` expand to `<0xc0ffee>`, which could either be a single `int` or an `array` of size `1` containing a 32-bit value.

In Zephyr, however, an array containing a **single** node reference has its own type **`phandle`**. In addition to the `phandle` type, three more types are available in Zephyr when dealing with `phandle`s and references:

| Zephyr type     | DTSpec equivalent      | Syntax                                   | Example                                |
| :-------------- | :--------------------- | :--------------------------------------- | :------------------------------------- |
| `path`          | `<prop-encoded-array>` | A plain node path string or reference    | `zephyr,console = &uart0;`             |
| `phandle`       | `<phandle>`            | An `array` containing a single reference | `pinctrl-0 = <&uart0_default>;`        |
| `phandles`      | `<prop-encoded-array>` | An array of references                   | `cpu-power-states = <&idle &suspend>;` |
| `phandle-array` | `<prop-encoded-array>` | An array containing references and cells | `gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;` |

Let's go through the remaining types one by one, starting with `path`s: In our previous example, we've enclosed the references in `<` and `>` and ended up with our `phandle` type. If we don't place the reference within `<` and `>`, the [DTSpec][devicetree-spec] defines the following behavior:

> "Outside a cell array, a reference to another node will be expanded to that node’s full path."

So let's try this out: Let's first get rid of the properties `phandle-by-path` and `phandle-by-label`, and create two new properties `path-by-path` and `path-by-label` as follows:

```dts
/ {
  label_a: node_a { /* Empty. */ };
  node_refs {
    path-by-path = &{/node_a};
    path-by-label = &label_a;
  };
};
```

If we now execute `west build`, we can see something interesting in our generated `zephyr.dts`:

`build/zephyr/zephyr.dts`
```dts
/dts-v1/;

/ {
  /* Lots of other nodes ... */
  label_a: node_a {
  };
  node_refs {
    path-by-path = &{/node_a};
    path-by-label = &label_a;
  };
};
```

The `phandle` property for `node_a` is gone! The reason for this is simple - and also matches exactly what is stated in the [DTSpec][devicetree-spec]: The references used for the values of the properties `path-by-path` and `path-by-label` are really just a different notation for the path `/node_a`. They are **not** `phandle`s.

In Zephyr, you'll encounter `path`s almost exclusively in the standard properties `/aliases` and `/chosen`, both of which we'll see in a [later section](#aliases-and-chosen-nodes).

Next, `phandles`. This is not a typo: The plural form `phandles` of `phandle` is really a separate type in Zephyr, and it is as simple as it sounds: Instead of supporting only a single _reference_ - or _phandle_ - in its value, it is an array of _phandles_. Let's create another `node_b` and add the property `phandles` to `node_refs`:


```dts
/ {
  label_a: node_a { };
  label_b: node_b { };
  node_refs {
    phandles = <&{/node_a} &label_b>;
  };
};
```
`build/zephyr/zephyr.dts`
```dts
/dts-v1/;

/ {
  /* Lots of other nodes ... */
  label_a: node_a {
    phandle = < 0x1c >;
  };
  label_b: node_b {
    phandle = < 0x1d >;
  };
  node_refs {
    phandles = < &{/node_a} &label_b >;
  };
};
```

The output is not surprising: Both nodes are now referenced by `node_ref`'s `phandles` and therefore get their own `phandle` property. The `phandles` type is useful when we need references to nodes of the same type, like the `cpu-power-states` example in the previous table.

This leaves us with the last type `phandle-array`, a type that is used quite frequently in devicetree. Let's go by example:

```dts
/ {
  label_a: node_a {
    #phandle-array-of-ref-cells = <2>;
  };
  label_b: node_b {
    #phandle-array-of-ref-cells = <1>;
  };
  node_refs {
    phandle-array-of-refs = <&{/node_a} 1 2 &label_b 3>;
  };
};
```
`build/zephyr/zephyr.dts`
```dts
/dts-v1/;

/ {
  /* Lots of other nodes ... */
  label_a: node_a {
    #phandle-array-of-ref-cells = < 0x2 >;
    phandle = < 0x1c >;
  };
  label_b: node_b {
    #phandle-array-of-ref-cells = < 0x1 >;
    phandle = < 0x1d >;
  };
  node_refs {
    phandle-array-of-refs = < &{/node_a} 0x1 0x2 &label_b 0x3 >;
  };
};
```

So what's this `phandle-array` type? It is simply a list of phandles _with metadata_. This is how it works:

- By convention, a `phandle-array` property is plural and its name should thus end in _s_.
- The value of a `phandle-array` property is an array of phandles, but each phandle can be followed by cells (32-bit values). In the example above, the two values `1 2` are `&{/node_a}`'s _metadata, whereas `3` is `&label_b`'s metadata.
- The new properties `#phandle-array-of-ref-cells` tell how many metadata _cells_ are supported by the node. Such properties are called [specifier cells][zephr-dts-bindings-specifier-cells]: In our example, for `node_a` it specifies that the node supports two cells, `node_b` only one.

The names of specifier cells like `#phandle-array-of-ref-cells` is fixed: It is formed by removing the plural '_s_' and attach '_-cells_' to the name of the `phandle-array` property. For our property _phandle-array-of-refs_, we thus end up with _phandle-array-of-ref~~s~~**-cells**_.

How is this used in practice? Let's look at the nRF52840's General Purpose input and outputs (GPIOs). In the datasheet we can find the following table on the GPIO instances:

| Base address | Peripheral | Instance | Description  | Configuration  |
| :----------: | :--------: | :------: | :----------- | :------------- |
|  0x50000000  |    GPIO    |    P0    | GPIO, port 0 | P0.00 to P0.31 |
|  0x50000300  |    GPIO    |    P1    | GPIO, port 1 | P1.00 to P1.15 |


In the nRF52840 DTS include file, we find the matching nodes `gpio0` for port _0_ and `gpio1` for port _1_:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  soc {
    gpio0: gpio@50000000 {
      compatible = "nordic,nrf-gpio";
      gpio-controller;
      reg = <0x50000000 0x200 0x50000500 0x300>;
      #gpio-cells = <2>;
      status = "disabled";
      port = <0>;
    };

    gpio1: gpio@50000300 {
      compatible = "nordic,nrf-gpio";
      gpio-controller;
      reg = <0x50000300 0x200 0x50000800 0x300>;
      #gpio-cells = <2>;
      ngpios = <16>;
      status = "disabled";
      port = <1>;
    };
  };
};
```

The nRF52840 development kit has LEDs, which are described using the node `leds` in the board's DTS file. Here we also see how the reference to the nRF52840's `gpio0` is used in the `uint8-array` called `gpios`:

`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
/ {
  leds {
    compatible = "gpio-leds";
    led0: led_0 {
      gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
      label = "Green LED 0";
    };
  };
};
```

There are no individual nodes for each pin, therefore, when referencing the `gpio0` node, we need to be able to tell exactly which pin we're using for our LED. In addition, we also might need to provide some configuration for our pin, e.g., set the pin to active low. Now, how would we know that `13` is the pin number and `GPIO_ACTIVE_LOW` are the flags?

I guess this is the time where I have to admit that I've been cheating you a bit. You might have noticed that yourself in case you modified my previous example with the `phandle-array-of-refs`: You could actually delete the `#phandle-array-of-ref-cells` properties, and also give `phandle-array-of-refs` a different name that does _not_ end in an _s_, and it will still compile just fine.

So what are we missing here? Well, without additional information, all that the DTS compiler can do is make sure the _syntax_ of your file is correct. It does not know anything about the **semantics** and therefore can't really associate the values in the `phandle-array` to `node_a` or `node_b`. It therefore also doesn't care about any semantic requirements. But how do we add semantics to our DTS files? Using the property `compatible` and so called **bindings**.

Before we look into devicetree semantics, let's wrap up the types.

### Summary of property types

TODO: here

| Zephyr type     | DTSpec equivalent                      | Syntax                                                                                                  | Example                                            |
| :-------------- | :------------------------------------- | :------------------------------------------------------------------------------------------------------ | :------------------------------------------------- |
| `boolean`       | `<empty>`                              | no value; a property is `true` if the property exists                                                   | `interrupt-controller;`                            |
| `string`        | `<string>`                             | double-quoted text (null terminated string)                                                             | `status = "disabled";`                             |
| `array`         | `<prop-encoded-array>`                 | 32-bit values enclosed in `<` and `>`, separated by spaces                                              | `reg = <0x40002000 0x1000>;`                       |
| `int`           | `<u32>`                                | a single 32-bit value ("cell"), enclosed in `<` and `>`                                                 | `current-speed = <115200>;`                        |
| `array`         | `<u64>`                                | 64-bit values are represented by an array of two *cells*                                                | `value = <0xBAADF00D 0xDEADBEEF>;`                 |
| `uint8-array`   | `<prop-encoded-array>` or "bytestring" | 8-bit hexadecimal values _without_ `0x` prefix, enclosed in `[` and `]`, separated by spaces (optional) | `mac-address = [ DE AD BE EF 12 34 ];`             |
| `string-array`  | `<stringlist>`                         | `string`s, separated by commas                                                                          | `compatible = "nordic,nrf-egu", "nordic,nrf-swi";` |
| `path`          | `<prop-encoded-array>`                 | A plain node path string or reference                                                                   | `zephyr,console = &uart0;`                         |
| `phandle`       | `<phandle>`                            | An `array` containing a single reference                                                                | `pinctrl-0 = <&uart0_default>;`                    |
| `phandles`      | `<prop-encoded-array>`                 | An array of references                                                                                  | `cpu-power-states = <&idle &suspend>;`             |
| `phandle-array` | `<prop-encoded-array>`                 | An array containing references and cells                                                                | `gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;`             |
| `compound`      | "comma-separated components"           | comma-separated values                                                                                  | `foo = <1 2>, [3, 4], "five"`                      |

TODO: complete examples
TODO: alternative syntax
TODO: extending nodes











Labels are only used in the devicetree source format and are not encoded into the DTB binary.
similarly, the phandle is not accessible in the Zephyr API but generates tokens.






`phandle` is a data type, a property and is usually also used interchangeably with references.


 [devicetree includes and sources](#devicetree-includes-and-sources-in-zephyr)


`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
&uart0 {
  compatible = "nordic,nrf-uarte";
  status = "okay";
  current-speed = <115200>;
  /* other properties */
};
```

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  soc {
    uart0: uart@40002000 {
      compatible = "nordic,nrf-uarte";
      reg = <0x40002000 0x1000>;
      interrupts = <2 NRF_DEFAULT_IRQ_PRIORITY>;
      status = "disabled";
    };
  };
};
```

`build/zephyr.dts`


The generated output for the Zephyr API we'll analyze in details [Devicetree and Zephyr, take two](#devicetree-and-zephyr-take-two)

TODO: labels within properties are allowed, but useless? does Zephyr generate a macro for it?




To sum it up:

[Zephyr phandles][zephyr-dts-phandles]



Zephyr uses `&label` as `phandle`, though `phandle` is actually a devicetree property.

Property values refer to other nodes in the devicetree by their phandles. You
foo: device@0 { };
device@1 {
        sibling = <&foo 1 2>;
};

In the devicetree, a phandle value is a cell – which again is just a 32-bit unsigned int. However, the Zephyr devicetree API generally exposes these values as node identifiers.














• In a cell array a reference to another node will be expanded to that node's phandle. References may be & followed
by a node's label
or they may be & followed by a node's full path in braces. Example:

Outside a cell array, a reference to another node will be expanded to that node's full path. Example: aliases and chosen, but that is a special node. and in props: path = &{/ctrl-1}; TODO: see what it expands to?


`zephyr/scripts/dts/python-devicetree/tests/test.dts`
```dts
/dts-v1/;

/ {
  props {
    compatible = "props";
    existent-boolean;
    int = <1>;
    array = <1 2 3>;
    uint8-array = [ 12 34 ];
    string = "foo";
    string-array = "foo", "bar", "baz";
    phandle-ref = < &{/ctrl-1} >;
    phandle-refs = < &{/ctrl-1} &{/ctrl-2} >;
    phandle-array-foos = < &{/ctrl-1} 1 &{/ctrl-2} 2 3 >;
    foo-gpios = < &{/ctrl-1} 1 >;
    path = &{/ctrl-1};
  };
};
```


### Semantics, `compatible` and bindings

Bindings: Without it, zephyr doesn't generate anything
[where are bindings found][zephyr-dts-bindings-location]

Bindings: in theory a node describes what it needs, but even without a binding
we get some output. Therefore, the syntax actually already defines the type !!!

Finally, we're able to identify the types of all properties of Nordic's `uart0` node:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  soc {
    uart0: uart@40002000 {
      compatible = "nordic,nrf-uarte";
      reg = <0x40002000 0x1000>;
      interrupts = <2 NRF_DEFAULT_IRQ_PRIORITY>;
      status = "disabled";
    };
  };
};
```

TODO: example for empty: interrupt-controller
zephyr/dts/arm/armv7-m.dtsi
TODO: example for bytearray jedec-id = [c2 28 17];
zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts

- `compatible` and `status` can either be `string`s or `string-array`s of size 1.
- `reg` is an `array` containing two 32-bit hexadecimal values.
- `interrupts` is an `array` containing a 32-bit hexadecimal value and the macro `NRF_DEFAULT_IRQ_PRIORITY`.








```dts
/dts-v1/;

/ {
  node1 {
    /* Booleans are properties without value assignments and are "true" if present. */
    a-boolean-true-property;
    /* Strings are \0 terminated. */
    a-string-property = "A string";
    /* String arrays are separated by commas. */
    a-string-array-property = "first string", "second string";
    /* Byte arrays are a list of 8-bit hexadecimal values. */
    a-byte-array-property = [0x01 0x23 0x34 0x56];
    /* A "cell" is really just a 32-bit value. */
    a-cell-property = <0xABCD>;
    /* Cell arrays are therefore lists where each value is 32-bit. */
    a-cell-array-property = <1 2 3 4>;
    /* Theoretically, devicetree also supports mixed properties. */
    /* a-mixed-property = "a string", [01 23 45 67], <0x12345678>; */
  };
  node2 {
    /* References to other nodes are also modelled as properties. */
    a-reference-property = <&node1>;
  };
};

/node2 {
  /* This extends the properties of node2. */
  a-boolean-true-property-for-node2;
};
```


TODO: prop-encoded-array -> (address, length)
array: simple list of values, though there is no real distinction
also all "prop-encoded-array" standard properties are really just numbers in the DTSpec.
y.

Overlays: Deleting booleans or pin assignments



<!-- #### Standard properties -->

Deleting properties to set a boolean to false or to free up assigned pins for unused features
`/delete-property/ whatever`
https://lists.zephyrproject.org/g/users/topic/removing_an_item_from_the/69717792
https://docs.zephyrproject.org/latest/build/dts/bindings-intro.html#where-bindings-are-located




TODO: try out things with devicetree compiler using an overlay. can be an arbitrary node, might help in the macrobatics?



### `aliases` and `chosen` nodes

aliases and chosen are actually required nodes
`zephyr/dts/common/skeleton.dtsi`

```dts
/dts-v1/;
/ {
  chosen {
  };
  aliases {
  };
};
```

Predefined nodes
`/cpus`, `/memory`

Nexus
The `#<specifier>-cells` property defines the number of cells required to encode a specifier for a domain.

Standard properties

* `phandle` is not really used when writing source files, it is only generated by the compiler.willnotcontainexplicitphandleproperties.TheDTCtoolautomatically inserts the phandle properties when the DTS is compiled into the binary DTB format.






















It is a _tree_

In contrast to Kconfig, Device Tree does not come with Zephyr but must be installed separately. So it is an external tool, a _compiler_

Like `Kconfig`, Device Tree is well established in the Linux kernel
Devicetree nodes are matched to bindings using their compatible properties.

binding, your driver C file can then use the devicetree API to find status = "okay" nodes with the desired compatible, and

"struct device" !! this is what it actually translates to. macrobatics is used for resolving the name to an instance.
https://docs.zephyrproject.org/latest/build/dts/howtos.html#write-device-drivers-using-devicetree-apis

Zephyr devicetree bindings are YAML files in a custom format (Zephyr does not use the dt-schema tools used by the Linux kernel).

construct called a devicetree to describe system hardware
point out similarity to JSON for tree

---

describes system hardware, thus the starting point is our hardware (board). conveniently, zephyr comes with a "description" of the nordic development kit (and an ever growing number of countless other devices)
nrf52840dk_nrf52840.dts

devicetree sources and devicetree bindings (.yml)
one is the model, the other "the devicetree" itself
"the devicetree" describes the hardware based on the model (bindings), and its initial configuration

to some degree similar to Kconfig, where the .conf files contain actual values, and the `Kconfig` files the model

[TODO:][zephyr-dts-intro-input-and-output]


* sources (.dts)
* includes (.dtsi)
* overlays (.overlay)
* bindings (.yaml)

only .yaml == bindings, rest is devicetree, just placed into different files that are used by the build process

by example UART:
previously known baudrate configuration, so where did that come from?




BNF
`zephyr/doc/build/dts/macros.bnf`
```
DT_N_<node path>_P_<property name>

path
; - each slash (/) to _S_
; - all letters to lowercase
; - non-alphanumerics characters to underscores

property-ID
; - all letters to lowercase
; - non-alphanumeric characters to underscores
e.g., zephyr,console -> zephyr_console

```

TODO: explore, delete `zephyr,console` from chosen nodes of the board, how does it fail?
how to find what "chosen" must/should be set?
https://docs.zephyrproject.org/latest/build/dts/api/api.html#zephyr-specific-chosen-nodes

### Summary of file types

`.overlay`
`.yaml`
`.dts`
`.dtsi`
`-pinctrl.dtsi`
`-qiaa.dtsi`

## Devicetree and Zephyr, take two

### Macrobatics

analyzing the macros: `_S_` is just a replacement for a forward slash `/`.

TODO: also, labels are possible in properties. Zephr DTS doesn't complain but the labels won't be generated

### The `_dt` API

### pinctrl ???

### Important properties such as status "okay"


analyze previous logging example in depth.

TODO: try to analyze UART since it also is a `chosen` node

TODO: instead of button, use uart overlay, and use button as pinctrl example.

## Summary

## Further reading

[nordicsemi]: https://www.nordicsemi.com/
[nordicsemi-academy-devicetree]: https://academy.nordicsemi.com/topic/devicetree/
[rpi-devicetree]: https://www.raspberrypi.com/documentation/computers/configuration.html#device-trees-overlays-and-parameters
[devicetree]: https://www.devicetree.org/
[devicetree-spec]: https://www.devicetree.org/specifications/
[linaro-devicetree]: https://www.linaro.org/blog/introducing-devicetree-org/
[zephyr-kconfig]: https://docs.zephyrproject.org/latest/build/kconfig/index.html#configuration-system-kconfig
[zephyr-dts]: https://docs.zephyrproject.org/latest/build/dts/index.html
[zephyr-dts-howto]: https://docs.zephyrproject.org/latest/build/dts/howtos.html
[zephyr-dts-bindings-api]: https://docs.zephyrproject.org/latest/build/dts/api/bindings.html
[zephyr-dts-bindings-types]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#type
[zephyr-dts-intro-bindings-properties]: https://docs.zephyrproject.org/latest/build/dts/intro-syntax-structure.html#properties
[zephyr-dts-intro-input-and-output]: https://docs.zephyrproject.org/latest/build/dts/intro-input-output.html
[zephyr-dts-intro-property-values]: https://docs.zephyrproject.org/latest/build/dts/intro-syntax-structure.html#writing-property-values
[zephyr-dts-phandles]: https://docs.zephyrproject.org/latest/build/dts/phandles.html
[zephyr-dts-overlays]: https://docs.zephyrproject.org/latest/build/dts/howtos.html#set-devicetree-overlays
[zephyr-dts-bindings-location]: https://docs.zephyrproject.org/latest/build/dts/bindings-intro.html#where-bindings-are-located
[zephr-dts-bindings-specifier-cells]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#specifier-cell-names-cells
[zephyr-summit-22-devicetree]: https://www.youtube.com/watch?v=w8GgP3h0M8M&list=PLzRQULb6-ipFDwFONbHu-Qb305hJR7ICe

basic logging



bindings: gpio
zephyr/dts/bindings/gpio/nordic,nrf-gpio.yaml
    zephyr/dts/bindings/gpio/gpio-controller.yaml
    zephyr/dts/bindings/base/base.yaml

KConfig
no spaces around the equals sign
application inherits the board configuration file, <board_name>_defconfig, of the board

build/zephyr/.config
west build --board nrf52840dk_nrf52840 -d ../build -t menuconfig