
- [Goals](#goals)
- [Prerequisites](#prerequisites)
- [What's a `devicetree`?](#whats-a-devicetree)
- [Devicetree and Zephyr](#devicetree-and-zephyr)
  - [CMake integration](#cmake-integration)
  - [Devicetree includes and sources in Zephyr](#devicetree-includes-and-sources-in-zephyr)
  - [Compiling the devicetree](#compiling-the-devicetree)
- [Basic source file syntax](#basic-source-file-syntax)
  - [Node names, unit-addresses, `reg` and labels](#node-names-unit-addresses-reg-and-labels)
  - [Property names and basic value types](#property-names-and-basic-value-types)
- [References and `phandle` types](#references-and-phandle-types)
  - [`phandle`](#phandle)
  - [`path`, `phandles` and `phandle-array`](#path-phandles-and-phandle-array)
    - [Syntax and semantics for `phandle-array`s](#syntax-and-semantics-for-phandle-arrays)
    - [`phandle-array` in practice](#phandle-array-in-practice)
- [A complete list of Zephyr's property types](#a-complete-list-of-zephyrs-property-types)
- [About `/aliases` and `/chosen`](#about-aliases-and-chosen)
  - [`/aliases`](#aliases)
  - [`/chosen`](#chosen)
- [Complete examples and alternative array syntax](#complete-examples-and-alternative-array-syntax)
- [Zephyr's DTS skeleton and addressing](#zephyrs-dts-skeleton-and-addressing)
- [Summary](#summary)
- [Further reading](#further-reading)

## Goals

TODO: DT basics and syntax, with a little bit of semantics

In the previous chapter, we've had a look at how to configure _software_, and we've silently assumed that there's a UART interface on our board that is configurable and used for logging.

In this chapter, we'll see how we configure and _use_ our peripherals. For this, Zephyr borrows another tool from the Linux kernel: the **devicetree** compiler. Similar to what we've seen with `Kconfig`, the `devicetree` has been adapted to better fit the needs of Zephyr when comparing it to Linux. We'll have a short look at this as well.

We'll leverage the fact that we're using a development kit with predefined peripherals, have a look at the configuration files that exist in Zephyr, and finally we'll see how we can change them to our needs.

In case you've already had an experience with `devictree`s, a short heads-up: We will _not_ define our own, custom board, and we will _not_ describe memory layouts. This is a more advanced topic and goes beyond this guide. With this chapter, we just want to have a detailed look and familiarize with the tool and files.

TODO: History? Honorable mention of Linaro?

Separating syntax from semantics since both at once can be a bit overwhelming, and for devicetree, repetition can't hurt, so we'll have _another_ look in the semantics chapter. More experienced readers might also find it awkward, but when learning I found the mention of "bindings" a bit annoying since i was still looking at the syntax.

## Prerequisites

TODO: This is part of a series, if you did not read the previous chapters, the least you need is a running project. we're using the Nordic devkit as a base to cheat the installation efforts.

TODO: Knowledge of `Kconfig` is assumed, Zephyr installed. If not, follow along the previous two chapters.

TODO: must know how to draft a zephyr freestanding application

TODO: accompanying repository, build root outside of application root.

some semantics for standard nodes and standard properties


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

To get started, we create a new freestanding application with the files listed below. In case you're not familiar with the required files, the installation, or the build process, have a look at the [previous](../00_basics/readme.md) [chapters](../01_kconfig/readme.md) or the official documentation. As mentioned in the [pre-requisites](#prerequisites), you should be familiar with creating, building and running a Zephr application.

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

But wait, we didn't "do anything devicetree" yet, did we? That's right, someone else already did it for us! After our [first look into Zephyr](../00_basics/readme.md) and after [exploring Kconfig](../01_kconfig/readme.md), we're familiar with the build output of our Zephyr application, so let's have a look! You should find a similar output right at the start of your build:

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

> **Note:** You might have noticed the file extensions `.dts` and `.dtsi`. Both file extensions are devicetree source files, but by convention the `.dtsi` extension is used for DTS files that are intended to be _included_ by other files.

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

> **Note:** In case this guide is too slow for you but you still want to know more about devicetree, there is a [brilliant video of the Zephyr Development Summit 2022 by Bolivar on devicetree][zephyr-summit-22-devicetree].

So if not a binary, what's the output of this `gen_defines.py` generator? Let's have another peek at the output of our build process:

```
-- Generated zephyr.dts: /path/to/build/zephyr/zephyr.dts
-- Generated devicetree_generated.h: /path/to/build/zephyr/include/generated/devicetree_generated.h
-- Including generated dts.cmake file: /path/to/build/zephyr/dts.cmake
```

We get three files: The `zephyr.dts` that has been generated out of our preprocessed `zephyr.dts.pre`, a `devicetree_generated.h` header file, and a CMake file `dts.cmake`.

As promised, the original devicetree `dtc` compiler _is_ invoked during the build, and that's where it comes into play: The `zephyr.dts` devicetree source file is fed into `dtc`, but not to generate any binaries or source code, but to generate warnings and errors. The output itself is discarded. This helps to reduce the complexity of the Python devicetree script `gen_defines.py` and ensures that the devicetree source file used in Zephyr is at least still compatible with the original specification.

The `devicetree_generated.h` header file replaces the devicetree blob `dtb`: It is included by the drivers and our application and thereby strips all unnecessary or unused parts. **"Macrobatics"** is the term that Martì Bolivar used in his [talk about the Zephyr devicetree in the June 2022 developer summit][zephyr-summit-22-devicetree], and it fits. Even for our tiny application, the generated header is over 15000 lines of code! We'll see later how these macros are used by the Zephyr API and drivers. If you're curious, have a look at `zephyr/include/zephyr/devicetree.h`, for now, let's have a glimpse:

`build/zephyr/include/generated/devicetree_generated.h`
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

That's it for peek into the Zephyr build. Let's wrap it up:
- Zephyr uses a so called devicetree to describe the hardware.
- Most of the devicetree sources are already available within Zephyr, e.g., MCUs and boards.
- We'll use devicetree mostly to override or extend existing DTS files.
- In Zephyr, the devicetree is resolved at _compile time_, using [macrobatics][zephyr-summit-22-devicetree].


## Basic source file syntax

Now we finally take a closer look at the devicetree syntax and its files. We'll walk through it by creating our own devicetree source file. This section heavily borrows from existing documentation such as the [devicetree specification][devicetree-spec], [Zephyr's devicetree docs][zephyr-dts] and the [nRF Connect SDK Fundamentals lesson on devicetree][nordicsemi-academy-devicetree].

### Node names, unit-addresses, `reg` and labels

TODO: "virtual", not really compiling?

Let's start from scratch. We create an empty devicetree source file `.dts` with the following empty tree:
TODO: details in [DEVICETREE SOURCE (DTS) FORMAT (VERSION 1)][devicetree-spec]

```dts
/dts-v1/;
/ { /* Empty. */ };
```

TODO: C style (`/* ... \*/`) and C++ style (`//`) comments are supported.

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

The _unit-address_ of each `uart` node matches its base address. The `reg` property seems to be some kind of value list enclosed in angle brackets `<...>`. The first value of the property matches the node's _unit-address_ and thus the base address of the UARTE instance, but the property also has a second value and thus provides more information than the _unit-address_ itself: Looking at the register map we can see that the second value `0x1000` matches the length of the address space that is reserved for each UARTE instance:

- The base address of the `UARTE0` instance is `0x40002000`, followed by `SPIM0` at `0x40003000`.
- The base address of the `UARTE1` instance is `0x40028000`, followed by `QSPI` at `0x40029000`.

Finally, each UART instance also has a unique label:
- `uart0` is the label of the node `/soc/uart@40002000`,
- `uart0` is the label of the node `/soc/uart@40028000`.

> **Note:** Throughout this chapter, we've sometimes refered to _node_ labels as just "labels". The [devicetree specification][devicetree-spec], however, allows labels to be placed also at other locations, e.g., in front of property values. The following is an example where we create a label for the second entry in an `array` value, and for a `string` value:
>
> ```dts
> / {
>   some_node {
>     array = <1 array_idx_one: 2 3>;
>     string = foo_value: "foo";
>   };
> };
> ```
>
> Zephyr's DTS generator accepts this input and won't complain about the additional labels. It will not, however, ignore such labels and therefore you can't use such labels in your application. We'll therefore use the term _label_ and _node label_ interchangeably.

### Property names and basic value types

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

## References and `phandle` types

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

### `phandle`

TODO: the easy answer: devicetree needs some way to refer to nodes, something like pointers in `C` ... that's what `phandle`s are. The typing system is a bit more complex, however, and distinguishes ...

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

### `path`, `phandles` and `phandle-array`

We've seen the type of the _phandle_ property, but what about the types of the properties `phandle-by-path` and `phandle-by-label`? Knowing that a `phandle` is expanded to a *cell* we might guess that `phandle-by-path` and `phandle-by-label` are of type `int` or `array`: Assuming that `node_a`'s _phandle_ has the value 0xc0ffee, both `<&{/node_a}>` and `<&label_a>` expand to `<0xc0ffee>`, which could either be a single `int` or an `array` of size `1` containing a 32-bit value.

In Zephyr, however, an array containing a **single** node reference has its own type **`phandle`**. In addition to the `phandle` type, three more types are available in Zephyr when dealing with `phandle`s and references:

| Zephyr type     | DTSpec equivalent      | Syntax                                   | Example                                |
| :-------------- | :--------------------- | :--------------------------------------- | :------------------------------------- |
| `path`          | `<prop-encoded-array>` | A plain node path string or reference    | `zephyr,console = &uart0;`             |
| `phandle`       | `<phandle>`            | An `array` containing a single reference | `pinctrl-0 = <&uart0_default>;`        |
| `phandles`      | `<prop-encoded-array>` | An array of references                   | `cpu-power-states = <&idle &suspend>;` |
| `phandle-array` | `<prop-encoded-array>` | An array containing references and cells | `gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;` |

Let's go through the remaining types one by one, starting with `path`s: In our previous example, we've enclosed the references in `<` and `>` and ended up with our `phandle` type. If we don't place the reference within `<` and `>`, the [DTSpec][devicetree-spec] defines the following behavior:

> "Outside a cell array, a reference to another node will be expanded to that node's full path."

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

In Zephyr, you'll encounter `path`s almost exclusively for properties of the standard nodes `/aliases` and `/chosen`, both of which we'll see in a [later section](#about-aliases-and-chosen).

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

_Specifier cells_ like `#phandle-array-of-ref-cells` have a defined naming convention: The name is formed by removing the plural '_s_' and attaching '_-cells_' to the name of the `phandle-array` property. For our property _phandle-array-of-refs_, we thus end up with _phandle-array-of-ref~~s~~**-cells**_.

You may have noticed that the property `#gpio-cells` doesn't follow the specifier cell naming convention: Every rule has its exceptions, and in case you're interested in details, I'll leave you with a reference to [Zephyr's documentation on specifier cells][zephr-dts-bindings-specifier-cells].

Before we look at a real-world example of a `phandle-array`, let's sum up `phandle` types. You'll find this exact list also in [Zephyr's documentation on `phandle` types][zephyr-dts-phandles], but since it is a short one, I hope they'll let me borrow it:

- To reference exactly one node, use the `phandle` type.
- To reference _zero_ or more nodes, we use the `phandles` type.
- To reference _zero_ or more nodes **with** metadata, we use a `phandle-array`.

#### Syntax and semantics for `phandle-array`s

In case you've been experimenting with the above overlay, you may have noticed that I've been cheating you a little: The project still _compiles_ just fine even if you delete the `#phandle-array-of-ref-cells` properties, or give `phandle-array-of-refs` a different name that does _not_ end in an _s_.

Why? Our devicetree is still _syntactically_ correct even if we do not follow the given convention. In the end, `phandle-array-of-refs` is simply an array of cells since every reference is expanded to the `phandle`'s value - even without the expansion, its syntax would still fit a `<prop-encoded-array`. A _syntactically_ sound devicetree, however, is only half the job: Eventually, we'll have to define some _schema_ and add _meaning_ to all the properties and their values; we'll have to define the devicetree's **semantics**.

Without semantics, the DTS generator can't make sense of the provided devicetree and therefore also won't generate anything that you'd be able to use in your application. Once you add semantics to your nodes, you'll have to strictly follow the previous convention, which is also why I've already included it in this chapter. The details, however, we'll explore in the next chapter about devicetree _semantics_.

#### `phandle-array` in practice

How are `phandle-array`s used in practice? Let's look at the nRF52840's _General Purpose input and outputs (GPIOs)_. In the datasheet, we can find the following table on the GPIO instances:

| Base address | Peripheral | Instance | Description  | Configuration  |
| :----------: | :--------: | :------: | :----------- | :------------- |
|  0x50000000  |    GPIO    |    P0    | GPIO, port 0 | P0.00 to P0.31 |
|  0x50000300  |    GPIO    |    P1    | GPIO, port 1 | P1.00 to P1.15 |

Thus, the nRF52840 uses two instances `P0` and `P1` of the GPIO peripheral to expose control over its GPIOs. In the nRF52840 DTS include file, we find the matching nodes `gpio0` for port _0_ and `gpio1` for port _1_:

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

You'll immediately notice that we're not able to use a reference to a single node in case we want to control only one pin: We always have to choose either `gpio0` or `gpio1` _and_ tell it which exact _pin_ of the _port_ we need. Let's see how this is solved in the nRF52840 development kit's devicetree file.

The nRF52840 development kit connects LEDs to the nRF52840 MCU, which are described using the node `leds` in the board's DTS file. Within this devicetree, we now see how the `led` instances reference to the nRF52840's GPIOs: E.g., `gpio0` is used in `led0`'s property `gpios` using a `phandle-array`:

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

There are no individual nodes for each pin, therefore, when referencing the `gpio0` node, we need to be able to tell exactly which pin we're using for our LED. In addition, we also typically need to provide some configuration for our pin, e.g., set the pin to _active low_. Now, how would we know that `13` is the pin number and `GPIO_ACTIVE_LOW` are the flags?

Well, as we've seen before, without additional information all that the DTS compiler can do is make sure the _syntax_ of your file is correct. It doesn't know anything about the **semantics** and therefore can't really associate the values in the `phandle-array` to `gpio0`. It therefore also doesn't care about any semantic requirements.

In the next chapter, we'll see how to use the standard property `compatible` and so called **bindings** to provide the semantics.

## A complete list of Zephyr's property types

Having explored `phandle` types and `paths`, we can complete the list of types that are used in Zephyr devicetrees. You can find the same information in [Zephyr's documentation on bindings][zephyr-dts-bindings-types] and [Zephyr's how-to on property values][zephyr-dts-intro-property-values].

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

It is again worth mentioning that the `compound` type is only a "catch-all" for custom types. Zephyr does **not** generate any macros for `compound` properties.


## About `/aliases` and `/chosen`

There are two standard _nodes_ that we need to mention: `aliases` and `chosen`. We'll see them again in the semantics chapter, but since they are quite prominent in Zephyr's DTS files, we can't just leave them aside for now.

### `/aliases`

Let's have a look at `/aliases` first. The [DTSpec][devicetree-spec] specifies that the `/aliases` node is a child node of the root node. The following is specified for its properties:

> Each property of the `/aliases` node defines an alias. The property _name_ specifies the _alias name_. The property _value_ specifies the _full **path** to a node in the devicetree_. [DTSpec][devicetree-spec]

Simply put, `/aliases` are just yet another way to get the full path to nodes _in your application_. In the [previous section](#path-phandles-and-phandle-array) we've learned that outside of `<` and `>` a reference to another node is expanded to that node's full path. Thus, for any alias we can specify its **`path` value** using references or a plain string, as shown in the example below:

```dts
/ {
  aliases {
    alias-by-label = &label_a;
    alias-by-path = &{/node_a};
    alias-as-string = "/node_a";
  };
  label_a: node_a {
    /* Empty. */
  };
};
```

So what's the point of having an alias? Can't we just use labels instead? Well, yes - but also no. As mentioned, _for the application_ there is no real difference between referring to a node via its label, its full path - or using its alias. We'll learn about the devicetree API in Zephyr in the next chapter; for now, know that there are three macros `DT_ALIAS`, `DT_LABEL`, and `DT_PATH` in `zephyr/include/zephyr/devicetree.h` that you can use to get a node identifier.

Why the emphasis on _the application_ and what exactly doesn't work? It's important to know that `/aliases` is just another node with properties that are compiled accordingly. You can**not** use aliases in the devicetree itself like you can do with _labels_. Thus, you can't replace an occurrence of a label by its alias. E.g., the following does not compile:

```dts
/ {
  aliases {
    alias-by-label = &label_a;
  };
  label_a: node_a {
    /* Empty. */
  };
  node_ref {
    // This doesn't work. An alias cannot be used like labels.
    phandle-by-alias = <&alias-by-label>;
  };
};
```

You might still not be convinced why you'd need an _alias_, so let's have a look at how they are used in Zephyr's DTS files. Say, we want to build an application that reads the state of a button. If we look at the nRF52840 development kit, we find the following nodes for the board's buttons:

`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
/ {
  buttons {
    compatible = "gpio-keys";
    button0: button_0 { /* ... */ };
    button1: button_1 { /* ... */ };
    button2: button_2 { /* ... */ };
    button3: button_3 { /* ... */ };
  };
};
```

In our application, we could refer to `button_0` via its full path `/buttons/button_0`, or using its label `button0`. Let's say we want to run the same application on a different board, e.g., STM's Nucleo-C031C6. After all, Zephyr's promise is that this should be easily doable, right? Let's have a look at its board's DTS file:

`zephyr/boards/arm/nucleo_c031c6/nucleo_c031c6.dts`
```dts
/ {
  gpio_keys {
    compatible = "gpio-keys";
    user_button: button { /* ... */ };
  };
};
```

So, our STM board has only one button, and for that board it has the label `user_button`. Notice that this is a perfectly fine label for the only available button on the board. However, if we'd like to use this button in our application, we'd now have to change our sources - or even worse - adapt the DTS files. Instead of doing this, we can use aliases - and since buttons are commonly used throughout Zephyr's example applications, the corresponding aliases already exist:

`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
/ {
  aliases {
    led0 = &led0;
    /* ... */
    pwm-led0 = &pwm_led0;
    sw0 = &button0;
    /* ... */
  };
};
```

`zephyr/boards/arm/nucleo_c031c6/nucleo_c031c6.dts`
```dts
/ {
  aliases {
    led0 = &green_led_4;
    pwm-led0 = &green_pwm_led;
    sw0 = &user_button;
    /* ... */
  };
};
```

> **Note:** In case the aliases wouldn't exist in the board DTS files, you could use overlay files - just like we already did in our examples. We'll explore this in detail in the next chapter.

As you can see, there are also other examples where labels are not consistent. Instead, aliases are used. We could change our application to get the node using the commonly available alias `sw0`, and it'll work with both boards.

### `/chosen`

Now what about the `/chosen` node? If you've been following along, or if you've just had another look at the DTS file of the nRF52840 development kit, then you might find that some of the `/chosen` nodes look an aweful lot like what you find in `/aliases`, just with a different property name format:

`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
/ {
  chosen {
    zephyr,console = &uart0;
    zephyr,shell-uart = &uart0;
    zephyr,uart-mcumgr = &uart0;
    zephyr,bt-mon-uart = &uart0;
    zephyr,bt-c2h-uart = &uart0;
    zephyr,sram = &sram0;
    zephyr,flash = &flash0;
    zephyr,code-partition = &slot0_partition;
    zephyr,ieee802154 = &ieee802154;
  };
};
```

So what's the difference to a property in `/aliases`? Let's first look at the definition of the `/chosen` node in the [DTSpec][devicetree-spec]:

> The `/chosen` node does not represent a real device in the system but describes parameters chosen or specified by the
system firmware at run time. It shall be a child of the root node. [DTSpec][devicetree-spec]

The first sentence can be a bit misleading: It doesn't mean that we cannot refer to real devices using `/chosen` properties, it simply means that a device defined as a `/chosen` property; it is always a _reference_. Thus, in short, `/chosen` contains a list of _system parameters_.

According to the [DTSpec][devicetree-spec], technically, `/chosen` properties are not restricted to the `path` type. The following are acceptable `/chosen` properties according to the specification, and Zephyr's DTS generator does indeed accept it as input:

```dts
/ {
  chosen {
    chosen-by-label = &label_a;
    chosen-by-path = &{/node_a};
    chosen-as-string = "/node_a";
    chosen-foo = "bar";
    chosen-bar = <0xF00>;
    chosen-invalid = "/invalid/path/is/a/string";
  };
  label_a: node_a {
    /* Empty. */
  };
};
```

At the time of writing, Zephyr uses `/chosen` properties exclusively for reference other _nodes_ and therefore the `/chosen` node only contains properties of the type `path`. Also, the `DT_CHOSEN` macro provided by `zephyr/include/zephyr/devicetree.h` is only used to retrieve node identifiers. You can find a list of all Zephyr-specific `/chosen` nodes [in the official documentation][zephyr-dts-api-chosen].

So when would you use `/aliases` and when `/chosen` properties? If you want to specify a _node_ that is independent of the nodes in a devicetree, you should use an _alias_ rather than a chosen property. `/chosen` is used to specify global configuration options and properties that affect the system as a whole.

In case you're building an application framework around Zephyr, you could also use `/chosen` properties for your own global configuration options. For simpler applications, you'll typically use `/aliases`. Think twice in case you're considering adding properties to `/chosen` that are **not nodes**: [Kconfig][zephyr-kconfig] is probably better suited for that. Zephyr's official documentation has a dedicated page on [Devicetree vs. Kconfig][zephyr-dt-vs-kconfig], check it out!

## Complete examples and alternative array syntax

TODO: link to this reference project.

Along with [the complete list of Zephyr's property types](#a-complete-list-of-zephyrs-property-types), we can now create two overlay files as complete examples for basic types, `phandle`s, `/aliases` and `/chosen` nodes.

`dts/playground/props-basics.overlay`
```dts
/ {
  aliases {
    // Aliases cannot be used as references in the devicetree itself, but are
    // used within the applicaiton as an alternative name for a node.
    alias-by-label = &label_equivalent;
    alias-by-path = &{/node_with_equivalent_arrays};
    alias-as-string = "/node_with_equivalent_arrays";
  };

  chosen {
    // `chosen` describes parameters "chosen" or specified by the application. In Zephyr,
    // all `chosen` parameters are paths to nodes.
    chosen-by-label = &label_equivalent;
    chosen-by-path = &{/node_with_equivalent_arrays};
    chosen-as-string = "/node_with_equivalent_arrays";
    // Technically, `chosen` properties can have any valid type.
    chosen-foo = "bar";
    chosen-bar = <0xF00>;
  };

  node_with_props {
    existent-boolean;
    int = <1>;
    array = <1 2 3>;
    uint8-array = [ 12 34 ];
    string = "foo";
    string-array = "foo", "bar", "baz";
  };
  label_equivalent: node_with_equivalent_arrays {
    // No spaces needed for uint8-array values.
    uint8-array = [ 1234 ];
    // Alternative syntax for arrays.
    array = <1>, <2>, <3>;
    int = <1>;
  };
};

// It is not possible to refer to a node via its alias - aliases are just properties!
// &alias-by-label {... };

// It is possible to "extend" and overwrite (non-const) properties of a node using
// its full path or its label.
&{/node_with_equivalent_arrays} {
  int = <2>;
};
&label_equivalent {
  string = "bar";
```

There are two things that we have already seen but haven't explicitly mentioned throughout this chapter: We can use node references outside the root node to define additional properties, and we can use a different syntax for specifying array values.

In the above example, the property `int` of the node `/node_with_equivalent_arrays` is overwritten with the value _2_, and a value `"bar"` is added to the node using its label `label_equivalent`.

In addition, we see that we don't have to provide all values of an `array` within a single set of `<..>`, but can instead separate them via comments. We also see the alternative syntax for `uint8-arrays`. In the semantics chapter, we'll see that the output is indeed the same, for now you have to trust me on this.

The following is the generated output for the two nodes `/node_with_props` and `/node_with_equivalent_arrays`:

`/build/zephyr/zephyr.dts`
```
/{
  node_with_props {
    existent-boolean;
    int = < 0x1 >;
    array = < 0x1 0x2 0x3 >;
    uint8-array = [ 12 34 ];
    string = "foo";
    string-array = "foo", "bar", "baz";
  };
  label_equivalent: node_with_equivalent_arrays {
    uint8-array = [ 12 34 ];
    array = < 0x1 >, < 0x2 >, < 0x3 >;
    int = < 0x2 >;
    string = "bar";
  };
};
```

The second overlay file shows the use of `path`s, `phandle`, `phandle`s and `phandle-array`, including the alternative array syntax that we've seen before. Here, for the property `phandle-array-of-refs`, you can see how this syntax can sometimes result in a more readable devicetree source file.

`dts/playground/props-phandles.overlay`
```dts
/ {
  label_a: node_a {
    // The value assignment for the cells is redundant in Zephyr, since
    // the binding already specifies all names and thus the size.
    #phandle-array-of-ref-cells = <2>;
  };
  label_b: node_b {
    #phandle-array-of-ref-cells = <1>;
  };
  node_refs {
    // Properties of type `path`
    path-by-path = &{/node_a};
    path-by-label = &label_a;

    // Properties of type `phandle`
    phandle-by-path = <&{/node_a}>;
    phandle-by-label = <&label_a>;

    // Array of phandle, type `phandles`
    phandles = <&{/node_a} &label_b>;
    // Array of phandles _with metadata_, type `phandle-array`
    phandle-array-of-refs = <&{/node_a} 1 2 &label_b 1>;
  };

  node_refs_equivalents {
    phandles = <&{/node_a}>, <&label_b>;
    phandle-array-of-refs = <&{/node_a} 1>, <2 &label_b 3>;
  };

  node_with_phandle {
    // It is allowed to explicitly provide the phandle's value, but the
    // DTS generator does this for us.
    phandle = <0xC0FFEE>;
  };
};
```

You can add those files to your own sources and provide both files using the `EXTRA_DTC_OVERLAY_FILE` option by separating the paths using a semicolon:

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build -- \
  -DEXTRA_DTC_OVERLAY_FILE="dts/playground/props-basics.overlay;dts/playground/props-phandles.overlay"
```

## Zephyr's DTS skeleton and addressing

Ok - we've seen nodes, labels, properties and value types _including_ `phandles`, we know how Zephyr compiles the devicetree and we've even seen the standard nodes `/aliases` and `/chosen`. Are we done yet? Almost. There's the famous _one last thing_ that is worth looking into before wrapping up on the devicetree _basics_. Yes, these are the basics, there's more coming in the next chapter!

> **Note:** Yes, we're again describing **semantics**, but one of our goals was being able to read Zephyr devicetree files. Without understanding the standard properties `#address-cells`, `#size-cells` and `reg`, we'd hardly be able to claim that. Also, their semantics is defined by the [devicetree specification][devicetree-spec].

When [exploring Zephyr's build](#devicetree-includes-and-sources-in-zephyr), we've seen that our DTS file include tree ends up including a skeleton file `zephyr/dts/common/skeleton.dtsi`. The following is a stripped down version of the full include tree that we've seen in the [introduction](#devicetree-includes-and-sources-in-zephyr) when building our empty application for the nRF52840 development kit:

```
nrf52840dk_nrf52840.dts
└── nordic/nrf52840_qiaa.dtsi
    └── nordic/nrf52840.dtsi
        └── arm/armv7-m.dtsi
            └── skeleton.dtsi
```

This `skeleton.dtsi` contains the minimal set of nodes and properties in a Zephyr devicetree:

```dts
/ {
  #address-cells = <1>;
  #size-cells = <1>;
  chosen { };
  aliases { };
};
```

We've already seen the `/chosen` and `/aliases` nodes, and the `#address-cells` and `#size-cells` properties look a lot like the [specifier cells][zephr-dts-bindings-specifier-cells] we've seen when looking at [the `phandle-array` type](#path-phandles-and-phandle-array), right?

Even though they match the naming convention, their purpose, however, is a different one: They provide the necessary _addressing_ information for nodes that use a _unit-addresses_. Too many new terms? Let's quickly repeat what we've already seen in the section about [node names](#node-names-unit-addresses-reg-and-labels):

- Nodes can have addressing information in their node name. Such nodes use the name format `node-name@unit-address` and **must** have the property `reg`.
- Nodes _without_ a _unit-address_ in the node name do _not_ have the property `reg`.

Let's first have a look at what we can find out about `reg` in the [DTSpec][devicetree-spec]:

> Property name `reg`, value type `<prop-encoded-array>` encoded as an arbitrary number of _(address, length)_ pairs.
>
> The `reg` property describes the address of the device’s resources within the address space defined by its parent [...]. Most commonly this means the offsets and lengths of memory-mapped IO register blocks [...].
>
> The value is a `<prop-encoded-array>`, composed of an arbitrary number of pairs of _address and length_, `<address length>`. The number of `<u32>` cells required to specify the address and length are [...] specified by the `#address-cells` and `#size-cells` properties in the parent of the device node. If the parent node specifies a value of _0_ for `#size-cells`, the length field in the value of `reg` shall be omitted.

Let's bring up our good old `uart@40002000` node from the nRF52840's DTS include file:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
#include <arm/armv7-m.dtsi>
#include "nrf_common.dtsi"

/ {
  soc {
    uart0: uart@40002000 { reg = <0x40002000 0x1000>; };
    uart1: uart@40028000 { reg = <0x40028000 0x1000>; };
  };
};
```

Now we should be able to tell for sure that `0x40002000` is the _address_ and `_0x1000` the _length_, right? Yes, but no: We've also learned that a `<u64>` value is represented using two cells, thus `<0x40002000 0x1000>` could technically be a single `<u64>` and _length_ could be omitted. To be _really_ sure how `uart@40002000` is addressed , we need to look at the parent node's `#address-cells` and `#size-cells` properties. So what are those properties? Let's look it up in the [DTSpec][devicetree-spec]:

> Property names `#address-cells`, `#size-cells`, value type `<u32>`.
>
> The `#address-cells` and `#size-cells` properties [...] describe how child device nodes should be addressed.
> - The `#address-cells` property defines the number of `<u32>` cells used to encode the **address** field in a child node’s `reg` property.
> - The `#size-cells` property defines the number of `<u32>` cells used to encode the **size** field in a child node’s `reg` property.
>
> The `#address-cells` and `#size-cells` properties are not inherited from ancestors in the devicetree. [...] A DTSpec-compliant boot program shall supply `#address-cells` and `#size-cells` on **all** nodes that have children. If missing, a client program should assume a default value of _2_ for `#address-cells`, and a value of _1_ for `#size-cells`.

Ok, now we're getting somewhere: We need to look at the parent's properties to know how many cells in the `reg` property's value are used to encode the _address_ and the _length_. In the DTS file `zephyr/dts/arm/nordic/nrf52840.dtsi`, however, there are no such properties for `soc`.

So, do we need to assume the defaults defined in the [DTSpec][devicetree-spec]? We might have to, but we've learned that node properties don't need to be provided in once place, and therefore the `#address-cells` and `#size-cells` of the `soc` might be provided by some includes.

Instead of going through all includes, we've seen that in Zephyr all includes are resolved by the preprocessor and therefore a single devicetree file is available in the build directory. You could therefore also find the combined `soc`'s properties in `build/zephyr/zephyr.dts`. Since our include graph is rather easy to traverse, though, we'll also find the missing information in the CPU architecture's DTS file:

`zephyr/dts/arm/armv7-m.dtsi`
```dts
#include "skeleton.dtsi"

/ {
  soc {
    #address-cells = <1>;
    #size-cells = <1>;
    /* ... */
  };
};
```

Now we really know that for our `uart@40002000` node, the `reg` value `<0x40002000 0x1000>` indeed refers to the _address_ `0x40002000` and the _length/size_ `0x1000`. This is also not surprising since the nRF52840 uses **32-bit architecture**.

We can also find an example for a **64-bit architecture** in Zephyr's DTS files:

`zephyr/dts/riscv/sifive/riscv64-fu740.dtsi`
```dts
/ {
  soc {
    #address-cells = <2>;
    #size-cells = <2>;

    uart0: serial@10010000 {
      reg = <0x0 0x10010000 0x0 0x1000>;
    };
  };
};
```

Here, `uart0`'s _address_ is formed by the `<u64>` value `<0x0 0x10010000>` and the _length_ by the `<u64>` value `<0x0 0x1000>`.

Addressing is not only used for register mapped devices, but, e.g., also for _bus_ addresses. For this, let's have a look at [Nordic's Thingy:53][nordicsemi-thingy53], a more complex board with several I2C peripherals connected to the nRF5340 MCU. The I2C bus of the nRF5340 uses the `#address-cells` and `#size-cells` properties to indicate that I2C nodes are uniquely identified via their address, no size is needed:

`zephyr/dts/arm/nordic/nrf5340_cpuapp_peripherals.dtsi`
```dts
i2c1: i2c@9000 {
  #address-cells = <1>;
  #size-cells = <0>;
};
```

In the _board_ DTS file, we find which I2C peripherals are actually hooked up to the I2C interface:
- The `BMM150` geomagnetic sensor uses the I2C address `0x10`,
- the `BH1749` ambient light sensor uses the I2C address `0x38`,
- and the `BME688` gas sensor the address `0x76`.

`zephyr/boards/arm/thingy53_nrf5340/thingy53_nrf5340_common.dts`
```dts
&i2c1 {
  bmm150: bmm150@10 { reg = <0x10>; };
  bh1749: bh1749@38 { reg = <0x38>; };
  bme688: bme688@76 { reg = <0x76>; };
};
```

With this, we're really done for our devicetree introduction.

## Summary

TODO: Zephyr specific devicetree stuff (`/chosen` are all nodes, `#include`, compile-time devicetree)

## Further reading

[nordicsemi]: https://www.nordicsemi.com/
[nordicsemi-academy-devicetree]: https://academy.nordicsemi.com/topic/devicetree/
[nordicsemi-thingy53]: https://www.nordicsemi.com/Products/Development-hardware/Nordic-Thingy-53
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
[zephyr-dts-api-chosen]: https://docs.zephyrproject.org/latest/build/dts/api/api.html#devicetree-chosen-nodes
[zephyr-kconfig]: https://docs.zephyrproject.org/latest/build/kconfig/index.html#configuration-system-kconfig
[zephyr-dt-vs-kconfig]: https://docs.zephyrproject.org/latest/build/dts/dt-vs-kconfig.html
[zephyr-summit-22-devicetree]: https://www.youtube.com/watch?v=w8GgP3h0M8M&list=PLzRQULb6-ipFDwFONbHu-Qb305hJR7ICe
<!-- [zephyr-thingy53]: https://docs.zephyrproject.org/latest/boards/arm/thingy53_nrf5340/doc/index.html -->

TODO: https://elinux.org/Device_Tree_Mysteries
