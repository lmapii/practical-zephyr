
- [Goals](#goals)
- [Prerequisites](#prerequisites)
- [Warm-up](#warm-up)
- [Devicetree overlays](#devicetree-overlays)
  - [Automatic overlays](#automatic-overlays)
  - [Overlays by example](#overlays-by-example)
- [Towards bindings](#towards-bindings)
  - [Extending the example application](#extending-the-example-application)
  - [Understanding devicetree macro names](#understanding-devicetree-macro-names)
  - [Matching `compatible` bindings](#matching-compatible-bindings)
  - [Bindings in Zephyr](#bindings-in-zephyr)
  - [Bindings directory](#bindings-directory)
- [Bindings by example](#bindings-by-example)
  - [Naming](#naming)
  - [Basic types](#basic-types)
    - [`boolean`](#boolean)
    - [`int`](#int)
    - [`array` and `uint8-array`](#array-and-uint8-array)
    - [`string`](#string)
    - [`string-array`](#string-array)
    - [`enum`](#enum)
  - [Labels and paths](#labels-and-paths)
    - [`aliases` and `chosen`](#aliases-and-chosen)
  - [Phandles](#phandles)
    - [`path`, `phandle` and `phandles`](#path-phandle-and-phandles)
    - [`phandle-array`](#phandle-array)
      - [`phandle-array` in Zephyr](#phandle-array-in-zephyr)
      - [A `phandle-array` from scratch](#a-phandle-array-from-scratch)
  - [Deleting properties](#deleting-properties)
  - [Full example](#full-example)
- [Zephyr's devicetree API](#zephyrs-devicetree-api)
  - [Macrobatics](#macrobatics)
  - [Node identifiers](#node-identifiers)
  - [Property values](#property-values)
- [Summary](#summary)
- [Further reading](#further-reading)

## Goals

TODO: rename to devicetree_bindings?

## Prerequisites

This builds up on devicetree basics

## Warm-up

<!-- TODO: name "easy pieces" ? -->

Before we get started, let's quickly review what we've seen when we had a look at the UART nodes of the [nRF52840 Development Kit from Nordic][nordicsemi]. We're using the same old freestanding application with an empty `prj.conf`, and the following file tree:

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

The `CMakeLists.txt` only includes the necessary boilerplate to create a freestanding Zephyr application. As application, we'll again use the same old `main` function that outputs the string _"Message in a bottle."_ each time it is called, and thus each time the device starts.

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

The following command builds this application for the [nRF52840 Development Kit from Nordic][nordicsemi] that we're using as reference. As usual, you can follow along with any board (or emulation target) and you should see a similar output.

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build
```

Using the `--board` parameter, Zephyr selects the matching devicetree source file `zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`, which specifies the parameters `status` and `current-speed` for the node with the label `uart0`, as follows:

`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
&uart0 {
  compatible = "nordic,nrf-uarte";
  status = "okay";
  current-speed = <115200>;
  /* other properties */
};
```

Zephyr's DTS generator script produces the matching output in `devicetree_generated.h` for the specified properties:

`build/zephyr/include/generated/devicetree_generated.h`
```c
#define DT_N_S_soc_S_uart_40002000_P_current_speed 115200
#define DT_N_S_soc_S_uart_40002000_P_status "okay"
```

Can we do the same for some devicetree nodes that we define from scratch? In the previous chapter, we've been using _overlay_ files to define custom nodes with their own properties to showcase Zephyr's devicetree types. Before we go ahead and make blind use of _overlays_ yet again, let's have a look at how they work.



## Devicetree overlays

<!-- In the previous chapter about the devicetree basics, we've had a detailed look at how the devicetree is handled in Zephyr's build process, its input and output files, and the resolution of the `C/C++` `#include` directives by the preprocessor. In case you've missed it, Zephyr's official documentation also has a [great overview about the input and ouput files][zephyr-dts-intro-input-and-output]. -->

_Overlays_ are used to extend or modify the board's devicetree source file. Even though - by convention - overlay files use the `.overlay` file extensions, they are also just plain DTS files. The build system combines the board's `.dts` file and any `.overlay` files by concatenating them, with the overlays put last. Thus, the contents of the `.overlay` file have priority over any definitions in the board's `.dts` file or its includes.

### Automatic overlays

The Zephyr build system automatically picks up additional _overlays_ based on their location and file name. The following list repeats the steps [in Zephyr's official documentation][zephyr-dts-overlays] used by the build system to detect _overlay_ files.

Before we have a look at the steps, there's one detail that we haven't seen yet. So far, we've specified the board as command line parameter in the format `--board <board>`. Zephyr also supports [building for a board revision][zephyr-build-board-revision] in case you have multiple _revisions_ or versions of a specific board. In such a case, you can use the format `--board <board>@<revision>`.

> **Note:** There are several ways to specify the board, but for the sake of simplicity we're only using `west` and its board parameter here. You can specify the board in your path using the variable `BOARD`, you can provide it directly in your `CMakeLists.txt` file, or even pass it to an explicit `cmake` call.

With this detail out of the way, here's the search performed by the CMake module `zephyr/cmake/modules/dts.cmake`:

- In case the CMake variable `DTC_OVERLAY_FILE` is set, the build system uses the specified file(s) and stops the search.
- If the file `boards/<BOARD>.overlay` exists in the application's root directory, the build system selects the provided file as overlay and proceeds with the following step.
- If a specific revision has been specified for the `BOARD` in the format `<board>@<revision>` and `boards/<BOARD>_<revision>.overlay` exists in the application's root directory, this file is used in _addition_ to `boards/<BOARD>.overlay`, if both exist.
- If _overlays_ have been encountered in any of the previous steps, the search stops.
- If no files have been found and `<BOARD>.overlay` exists in the application's root directory, the build system uses the overlay and stops the search.
- Finally, if none of the above overlay files exist but `app.overlay` exists in the application's root directory, the build system uses the overlay.

On top of the _overlay_ files that have or haven't been discovered by the build process, the CMake variable `EXTRA_DTC_OVERLAY_FILE` allows to specify additional _overlay_ files that are added regardless of the outcome of the overlay search.

The important thing to remember is, that the devicetree overlay files that have been detected _last_ have the _highest_ precedence, since they may overwrite anything of the previously added overlay files. The precedence is always visible in the build output, where Zephr lists all overlay files using the output `Found devicetree overlay: <name>.overlay` in the order that they are detected and thus added. The precedence _increases_ with the given list.

### Overlays by example

Let's try and visualize this list using an imaginary filetree and board "dummy_board". I've annotated the files with precedence numbers, even though obviously not all files will be used by the build:

```bash
$ tree --charset=utf-8 --dirsfirst.
.
├── boards
│   ├── dummy_board_123.overlay -- #3
│   └── dummy_board.overlay     -- #4
├── dts
│   └── extra
│       ├── extra_0.overlay -- #2
│       └── extra_1.overlay -- #1
├── src
│   └── main.c
├── CMakeLists.txt
├── app.overlay         -- #6
├── dummy_board.overlay -- #5
└── prj.conf
```

Let's assume we would use the following command that doesn't include `DTC_OVERLAY_FILE`:

```bash
$ west build --board dummy_board@123 -- \
  -DEXTRA_DTC_OVERLAY_FILE="dts/extra/extra_0.overlay;dts/extra/extra_1.overlay"
```

The overlay files would be detected and added as follows, depending on whether or not they exist. As mentioned, the precedence _increases_ and thus the files listed _last_ have the _highest_ precedence:

- `boards/dummy_board.overlay`
- `boards/dummy_board_123.overlay`
- if none of the previous files exist, `dummy_board.overlay`
- if none of the previous files exist, `app.overlay`
- `dts/extra/extra_0.overlay`
- `dts/extra/extra_1.overlay`

> **Note:** It is recommended to use the `boards` directory for board overlay files. You should no longer place your board's overlay files in the application's root directory.

If, instead, we'd specify the CMake variable `DTC_OVERLAY_FILE` as `app.overlay` in our command as follows, the automatic detection is skipped and the build process only picks the selected DTC overlay files:

```bash
$ west build --board dummy_board@123 -- \
  -DTC_OVERLAY_FILE="app.overlay" -DEXTRA_DTC_OVERLAY_FILE="dts/extra/extra_0.overlay;dts/extra/extra_1.overlay"
```

The overlay files would thus be added as follows and with _increasing_ precedence:
- `app.overlay`
- `dts/extra/extra_0.overlay`
- `dts/extra/extra_1.overlay`

<!-- TODO: in words: app.overlay for quick overlays, they won't be used anymore once you have an application that should work for multiple boards. thus, extra overlays are a good practice -->



## Towards bindings

Now that we finally know what _overlays_ are and how we can use them in our build, let's find out what Zephyr produces for our own overlays in its `devicetree_generated.h` file.

### Extending the example application

We start by creating our own overlay file `dts/playground/props-basics.overlay`:

```bash
$ tree --charset=utf-8 --dirsfirst.
├── dts
│   └── playground
│       └── props-basics.overlay
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

In the development kit's devicetree source file `nrf52840dk_nrf52840.dts`, the `&uart0` node has two properties: A property `current-speed` of type `int`, and the property `status` of type `string`. We've seen that Zephyr creates some output in `devicetree_generated.h` just fine, let's try the same with our own custom node:

`dts/playground/props-basics.overlay`
```dts
/ {
  node_with_props {
    int = <1>;
    string = "foo";
  };
};
```

When running a build whilst specifying the path to the overlay using the CMake variable `EXTRA_DTC_OVERLAY_FILE`, we can verify that the overlay is indeed picked up by the build system, since it informs us about the overlays it found using the output `Found devicetree overlay`:

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build -- \
  -DEXTRA_DTC_OVERLAY_FILE="dts/playground/props-basics.overlay"
```
```
-- Found Dtc: /opt/nordic/ncs/toolchains/4ef6631da0/bin/dtc (found suitable version "1.6.1", minimum required is "1.4.6")
-- Found BOARD.dts: /opt/nordic/ncs/v2.4.0/zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts
-- Found devicetree overlay: dts/playground/props-basics.overlay
-- Generated zephyr.dts: /path/to/build/zephyr/zephyr.dts
-- Generated devicetree_generated.h: /path/to/build/zephyr/include/generated/devicetree_generated.h
-- Including generated dts.cmake file: /path/to/build/zephyr/dts.cmake
```

Checking the output file `build/zephyr/zephyr.dts` we also see that our `node_with_props` can be found in the devicetree that Zephyr is using as input for `devicetree_generated.h`:

`build/zephyr/zephyr.dts`
```dts
/{
  /* ... */
  node_with_props {
    int = < 0x1 >;
    string = "foo";
  };
};
```

Let's see if we can find anything in `devicetree_generated.h` by using the quite unique value `foo` of our `string` property:

```bash
$ grep foo ../build/zephyr/include/generated/devicetree_generated.h
$
```

The search yields no results! Did we miss something? One thing that is easy to check, is that the `devicetree_generated.h` contains our node: We can just search using the node's full path `/node_with_props`. You should find a large comment, separating the macros generated for our node, containing a list of definitions:

`build/zephyr/include/generated/devicetree_generated.h`
```c
/*
 * Devicetree node: /node_with_props
 *
 * Node identifier: DT_N_S_node_with_props
 */

/* Node's full path: */
#define DT_N_S_node_with_props_PATH "/node_with_props"
// ---snip ---
/* (No generic property macros) */
```

The omitted lines contain lots of generic macros for our node. Don't worry, we'll skim through these soon enough. However, we won't find anything that is even remotely related to the two properties that we defined for our node. In fact, the comment _"(No generic property macros)"_ seems to hint that the generator did not encounter the properties `int` and `string` in our node `/node_with_props`.

### Understanding devicetree macro names

Before we dig deeper, let's try to gain a better understanding of the macro names in `devicetree_generated.h`. Once we understand those, we should be able to know which macro or macros Zephyr should generate for our node's properties.

In Zephyr's `doc` folder, you can find the _"RFC 7405 ABNF grammar for devicetree macros"_ `zephyr/doc/build/dts/macros.bnf`. This RFC describes the macros that are directly generated out of the devicetree. In simple words, the following rules apply:

- `DT` is the common prefix for devicetree macros,
- `S` is a forward slash `/`,
- `N` refers to a _node_,
- `P` is a _property_,
- all letters are converted to lowercase,
- and non-alphanumerics characters are converted to underscores "`_`"

Let's look at the same old `/soc/uart@40002000` node, specified in the nRF52840's DTS file, and modified by the nRF52840 development kit's DTS file:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  soc {
    uart0: uart@40002000 {
      compatible = "nordic,nrf-uarte";
      reg = <0x40002000 0x1000>;
    };
  };
};
```

`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
&uart0 {
  compatible = "nordic,nrf-uarte";
  status = "okay";
  current-speed = <115200>;
  /* other properties */
};
```

Following the previous rules, we can transform the paths and property names:
- The node path `/soc/uart@40002000` is transformed to `_S_soc_S_uart_40002000`.
- The property name `current-speed` is transformed to `current_speed`.
- The property name `status` stays the same.

Since node paths are unique, by combining the node's path with its property names we can create a unique macro for each property - and that's exactly what the devicetree generator does. The leading `_N_` is used to indicate that it is followed by a node's path, `_P_` separates the node's path from its property. For `uart@40002000`, we get the following:

- `/soc/uart@40002000`, property `current-speed`
  becomes `N_S_soc_S_uart_40002000_P_current_speed`
- `/soc/uart@40002000`, property `status`
  becomes `N_S_soc_S_uart_40002000_P_status`

Adding the `DT_` prefix, we can indeed find those macros in `devicetree_generated`:

`build/zephyr/include/generated/devicetree_generated.h`
```c
#define DT_N_S_soc_S_uart_40002000_P_current_speed 115200
#define DT_N_S_soc_S_uart_40002000_P_status "okay"
```

For our little demo overlay we can do the same:

`dts/playground/props-basics.overlay`
```dts
/ {
  node_with_props {
    int = <1>;
    string = "foo";
  };
};
```

For `/node_with_props`' properties, the generator should create the following macros:
- `DT_N_S_node_with_props_P_int` for the property `int`,
- `DT_N_S_node_with_props_P_string` for the property `string`.

Thus, in our `devicetree_generated.h` we should be able to find the above macros - but we don't. Something's still missing.

### Matching `compatible` bindings

What's the difference between `/soc/uart@40002000` and our `/node_with_props`? There are lots, but the significant difference is, that `/soc/uart@40002000` has the **`compatible`** property. `compatible` is a standard property defined in the [DTSpec][devicetree-spec], so let's look it up:

> Property name `compatible`, value type `<stringlist>`.
>
> The `compatible` property value consists of one or more strings that define the specific programming model for the device. This list of strings should be used by a client program for device driver selection. The property value consists of a concatenated list of null terminated strings, from most specific to most general. They allow a device to express its compatibility with a family of similar devices [...].
>
> The recommended format is `"manufacturer,model"`. [...] The compatible string should consist only of lowercase letters, digits and dashes, and should start with a letter. [...]

Let's rephrase this: `compatible` is a list of strings, where each string is essentially a reference to some _model_. The [DTSpec][devicetree-spec] uses a specific term for such models: They are called **bindings**.

The [DTSpec][devicetree-spec] defines _bindings_ as _"requirements [...] for how specific types and classes of devices are represented in the devicetree."_ In very simple words, a binding defines the properties that a node (or its children) can or even must have, their exact type, and their **meaning**.

How is this any different from what we've been doing until now in our devicetree source files? Well, without a _binding_, we can give a node any number of properties, and can assign any property any value of any type. Also, any property name must be considered random. Let's take our own `node_with_props`:

`dts/playground/props-basics.overlay`
```dts
/ {
  node_with_props {
    int = <1>;
    string = "foo";
  };
};
```

The devicetree compiler won't complain if we'd assign a `string` to the property `int`. In fact, it doesn't even know whether or not `node_with_props` should have this property at all. We could even delete it and the compiler won't complain. We also don't know what the purpose of `int` or `string` mean or what they're used for. The same is true for the `current-speed` property of the `/soc/uart@40002000` node in the nRF52840 devicetree: Without any additional information, we can only _assume_ that this is the baud rate in bits/s, but we can't know for sure.

Thus, by specifying _compatible bindings_, we're telling the devicetree compiler to check whether the given node really matches the properties and types defined in the _binding_, and we're telling it what to do with the information provided in the devicetree. Bindings also add the **semantics to a devicetree** by giving properties a _meaning_.

The [DTSpec][devicetree-spec] includes some standard bindings, e.g., bindings for serial devices such as our UART device. It thus defines how serial devices should look like in the devicetree. This binding includes the `current-speed` property:

|             |                                                                                                                                                     |
| :---------- | :-------------------------------------------------------------------------------------------------------------------------------------------------- |
| Property    | `current-speed`                                                                                                                                     |
| Value type  | `<u32>`                                                                                                                                             |
| Description | Specifies the current speed of a serial device in bits per second. A boot program should set this property if it has initialized the serial device. |
| Example     | 115,200 Baud: `current-speed = <115200>;`                                                                                                           |

<!-- > **Note:** In case you're wondering why `current-speed` is not listed in the _"Standard Properties"_ section in the [DTSpec][devicetree-spec], but all of a sudden appears in the section about _"Serial devices" in the "Device Bindings", you're not alone: TODO: -->

Finally, we're getting somewhere! We now know what the `current-speed` property is all about: We know its type, its physical unit, and its _meaning_. But this is just some table in the [DTSpec][devicetree-spec], how is this information represented in Zephyr?

_Bindings_ live outside the devicetree. The [DTSpec][devicetree-spec] doesn't specify any file format or syntax that is used for bindings. Zephyr, like [Linux][linux-dts-bindings], uses `.yaml` files for its bindings. I'm assuming you're familiar with `YAML` - in case you're not, have a quick look at its [online documentation][yaml].

### Bindings in Zephyr

[Zephyr's official documentation][zephyr-dts-bindings-syntax] provides a comprehensive explanation of the syntax used for bindings. Unless you're adding your own devices and drivers, you'll hardly ever need to create bindings yourself. Therefore, in this section we'll walk through some existing bindings in Zephyr to gain a general understanding, and then create [our own example bindings](#bindings-by-example) in the next section.

Let's have a look at what we can find out about `/soc/uart@40002000`'s `current-speed` and `status` properties in Zephyr: Via its `compatible` property, the node `/soc/uart@40002000` claims compatibility with the binding `nordic,nrf-uarte`, and thus `nordic`'s model (binding) for `nrf-uarte` devices:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  soc {
    uart0: uart@40002000 {
      compatible = "nordic,nrf-uarte";
      /* ... */
    };
  };
};
```

Zephyr recursively looks for devicetree bindings in `zephyr/dts/bindings`. Bindings are matched against the strings provided in the `compatible` property. Thus, for our UART node, Zephyr looks for a binding that matches `nordic,nrf-uarte.yaml`. Conveniently, bindings in Zephyr use the same basename as the `compatible` string, and thus we can find the correct binding by searching for a file called `nordic,nrf-uarte.yaml`, which can be found in the `serial` bindings subfolder:

`zephyr/dts/bindings/serial/nordic,nrf-uarte.yaml`
```yaml
description: Nordic nRF family UARTE (UART with EasyDMA)
compatible: "nordic,nrf-uarte"
include: ["nordic,nrf-uart-common.yaml", "memory-region.yaml"]
```

Checking the _compatible_ key, we see that it indeed matches the node's property. Notice that this key's value is what is matched against the property, _not_ the filename. Theoretically, the file could have a different name, but by convention matches the _compatible_ key.

Apart from _compatible_, the binding also has a textual _description_ and some *include*s. As the name suggests, the _include_ key allows to include the content of other bindings. Files are included by filename without specifying any paths or directories - Zephyr determines the search paths, one of which includes all subfolders of `zephyr/dts/bindings`. For the exact syntax, have a look at the [official documentation][zephyr-dts-bindings-syntax-include].

The contents of included files are essentially merged using a recursive dictionary merge. In short: Everything that's declared in included bindings is available in the including file.

> **Note:** In case you're wondering what happens with duplicated keys and/or values, try it out based on what we learn in the next section, where we'll create our own bindings.

Since `nordic,nrf-uarte.yaml` doesn't seem to define anything related to our properties - just like in the chapter on `Kconfig` - we once again have to walk down the include tree. Let's try `nordic,nrf-uart-common.yaml`:

`zephyr/dts/bindings/serial/nordic,nrf-uart-common.yaml`
```yaml
include: [uart-controller.yaml, pinctrl-device.yaml]
properties:
  # --snip--
  current-speed:
    description: |
      Initial baud rate setting for UART. Only a fixed set of baud
      rates are selectable on these devices.
    enum:
      - 1200
      - 2400
      # --snip--
      - 921600
      - 1000000
```

This must be it! There's now a key _properties_, which a child element that matches our `current-speed` property. And sure enough, the _properties_ key is used to [properties that nodes which match the binding contain][zephyr-dts-bindings-syntax-properties].

Here, `nordic` seems to restrict the allowed baudrates using an *enum*eration. We can therefore only specify values from the given list. But how does the devicetree compiler know the type of the property? Couldn't we also use an *enum*eration to pre-define `string`s? Yes, we could, and there's indeed something missing: The property's _type_.

To find out the type, we need to step further down the include tree, into `uart-controller.yaml`. This is Zephyr's base model for UART controllers, which is used regardless of the actual manufacturer:

`zephyr/dts/bindings/serial/uart-controller.yaml`
```yaml
include: base.yaml

bus: uart

properties:
  # --snip--
  current-speed:
    type: int
    description: Initial baud rate setting for UART
  # --snip--
```

Now, we finally know that `current-speed` is of type `int` and is used to configure the initial baud rate setting for UART (though the descriptio fails to mention that the baud rate is specified in _bits per second_). We can only select from a pre-defined list of baud rates and cannot specify our own custom baud rate - at least not in the devicetree. Given this _binding_, the devicetree compiler now rejects any but the allowed values, and it is therefore not possible to specify a syntactically correct value that is not an integer of the given list.

> **Note:** The `bus: uart` is a special key that allows you to associate devices to a bus system, e.g., I2C, SPI or UART. This feature is especially useful if a device supports multiple bus types, e.g., a sensor that can be connected via SPI or I2C. This is out of scope for this chapter, though, but is explained perfectly in the [official documentation][zephyr-dts-bindings-syntax-bus].

What about the `status`? As you might have guessed, we need to take yet another step down the include tree and have a look at the `base.yml` binding. This binding contains common fields used by _all_ devices in Zephyr. Here, we not only encounter the `status` property, but also the `compatible` property:

`zephyr/dts/bindings/base/base.yaml`
```yaml
include: [pm.yaml]

properties:
  status:
    type: string
    description: indicates the operational status of a device
    enum:
      - "ok" # Deprecated form
      - "okay"
      - "disabled"
      - "reserved"
      - "fail"
      - "fail-sss"

  compatible:
    type: string-array
    required: true
    description: compatible strings
  # --snip--
```

Thus, _status_ is simply a property of type `string` with pre-defined values that can be assigned in the devicetree. It indicates the operational status of a device, which we'll see in action in a later section.

While going through binding files may seem tedious, the include tree depth is usually quite small, and once you know the base bindings such as `base.yaml`, it typically comes down to a handfull of files. There is, however, no "flattened" output file like `build/zephyr/zephyr.dts`, and therefore it is always necessary to walk through the bindings. We'll have a quick look at [Nordic's][nordicsemi] plugin for _Visual Studio Code_ later, but thus far you'll always need to look at the source files. The following shows the include tree of `nordic,nrf-uarte.yaml` at the time of writing:

```
nordic,nrf-uarte.yaml
├── nordic,nrf-uart-common.yaml
│   ├── uart-controller.yaml
│   │   └── base.yaml
│   │       └── pm.yaml
│   └── pinctrl-device.yaml
└── memory-region.yaml
```

There's one more important thing about `compatible` and matching bindings: If a node has more than one string in its `compatible` property, the build system looks for compatible bindings in the listed order and uses the first match.

### Bindings directory

Before we can go ahead and experiment using our own bindings, we need to know where to place them. Just like the `dts/bindings` in _Zephyr's_ root directory, the [build process][zephyr-dts-intro-input-and-output] also picks up any bindings in `dts/bindings` in the _application's_ root directory.

In contrast to overlays, however, detected bindings are not listed in the build output.


## Bindings by example

This is it, we're finally there! We'll now add bindings for our [extended example application](#extending-the-example-application) to get some generated output for our node's properties. We've just seen that we can place our bindings in the [`dts/bindings` directory](#bindings-directory).

### Naming

Before we start we need to solve one of the hardest problems in engineering: Finding a good _name_. The [devicetree specification][devicetree-spec] contains a guideline on the value for the `compatible` property of a node - and therefore the name of the binding:

> "The `compatible` string should consist only of lowercase letters, digits and dashes, and should start with a letter. A single comma is typically only used following a vendor prefix. Underscores should not be used." [DTSpec][devicetree-spec]

Let's try this with the binding name _custom,props-basic_ and thus vendor prefix _custom_. We'll follow the convention and use the binding's name as filename and create a new file `custom,props-basics.yaml` in the application's `dts/bindings` directory:

```bash
$ tree --charset=utf-8 --dirsfirst.
├── dts
│   ├── bindings
│   │   └── custom,props-basics.yaml
│   └── playground
│       └── props-basics.overlay
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

In our binding, we define our `compatible` key as `"custom,props-basics"`, and define two properties `int` and `string` of the matching type, without providing any description:

`dts/bindings/custom,props-basics.yaml`
```yaml
description: Custom properties
compatible: "custom,props-basics"

properties:
  int:
    type: int
  string:
    type: string
```

Bindings define a node's properties under the key _properties_. The above is the simplest form for a property in a binding, and has the following form:

```yaml
properties:
  <property-name>:
    type: <property-type>
    # required: false -> omitted by convention if false
```

Properties have a _name_ and are therefore unique within the node, and each property is assigned a _type_ using the corresponding key. Other keys are such as _required_ are optional.

There are several other keys and "features", e.g., it is possible to define the properties for _children_ of a node with the matching `compatible` property, but we'll only have a look at the very basics. Definitely dive into [Zephyr's official documentation][zephyr-dts-bindings-syntax] once you're through with this chapter!

Finally, we create a new property `compatible = "custom,props-basic"` for our existing `node_with_props` ...

`dts/playground/props-basics.overlay`
```dts
/ {
  node_with_props {
    compatible = "custom,props-basic"
    int = <1>;
    string = "foo bar baz";
  };
};
```

... and recompile:

```bash
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 --build-dir ../build -- \
  -DEXTRA_DTC_OVERLAY_FILE="dts/playground/props-basics.overlay"
```
```
-- Found devicetree overlay: dts/playground/props-basics.overlay
node '/node_with_props' compatible 'dummy,props-basics' has unknown vendor prefix 'dummy'
-- Generated zephyr.dts: /path/to/build/zephyr/zephyr.dts
-- Generated devicetree_generated.h: /path/to/build/zephyr/include/generated/devicetree_generat
```

> **Note:** As mentioned, at the time of writing and in contrast to overlay files, bindings are not listed in the build output, not even for bindings in the application's `dts/bindings` directory.

It seems that the devicetree compiler is not too happy about our _"dummy"_ vendor prefix. Zephyr warns us here since it maintains a _"devicetree binding vendor prefix registry"_ `zephyr/dts/bindings/vendor-prefixes.txt` to avoid name-space collisions for properties and bindings. If you're a vendor, you can of course try to add your name upstream, but we'll skip the vendor prefix and use the binding name _custom-props-basics_ instead.

```bash
$ mv dts/bindings/dummy,props-basics.yaml dts/bindings/custom-props-basics.yaml
$ sed -i .bak 's/dummy,/custom-/g' dts/bindings/custom-props-basics.yaml
$ sed -i .bak 's/dummy,/custom-/g' dts/playground/props-basics.overlay
$ rm dts/**/*.bak

$ tree --charset=utf-8 --dirsfirst.
├── dts
│   ├── bindings
│   │   └── custom-props-basics.yaml
│   └── playground
│       └── props-basics.overlay
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

Without recompiling, we can check whether the generator script has added our properties to `devicetree_generated.h`. The value _foo_ is unique enough for a quick `grep`; for our property _int_ we'll use what we've learned and expect finding some macro containing `node_with_props_P_int`. And indeed, we've **finally** have our generated output!

```bash
$ grep foo ../build/zephyr/include/generated/devicetree_generated.h
#define DT_N_S_node_with_props_P_string "foo bar baz"
#define DT_N_S_node_with_props_P_string_STRING_UNQUOTED foo bar baz
#define DT_N_S_node_with_props_P_string_STRING_TOKEN foo_bar_baz
$ grep node_with_props_P_int ../build/zephyr/include/generated/devicetree_generated.h
#define DT_N_S_node_with_props_P_int 1
#define DT_N_S_node_with_props_P_int_EXISTS 1
```

We'll learn how to use those macros in the section about [Zephyr's devicetree API](#zephyrs-devicetree-api). For now, we'll just make sure that our bindings lead to some generaetd output for all supported types. In case you can't wait and want to have a more detailed look instead, I suggest you have a look at [Zephyr's brilliant introduction to devicetree bindings][zephyr-dts-bindings-intro].

> **Note:** If you're experimenting and don't see any output, make sure that the `compatible` property is set correctly. The devicetree compiler does **not** complain in case it doesn't find a matching binding. So in case you have a typo in your `compatible` property, the application builds without warnings, but `devicetree_generated.h` won't have any content for the desired properties.

### Basic types

In the previous chapter about devicetree, we've seen all basic types. We'll use the same node, but extend it with two properties called `enum-int` and `enum-string`, since *enum*erations are represented differently in bindings. In the previous chapter about devicetree basics we've seen that Zephyr's devicetree generator ignores _value_ labels. We'll throw in two of those, named `second_value` and `string_value`, for good practice, and also add the `label_with_props` for `/node_with_props`:

`dts/playground/props-basics.overlay`
```dts
/ {
  label_with_props: node_with_props {
    compatible = "custom-props-basics";
    existent-boolean;
    int = <1>;
    array = <1 second_value: 2 3>;
    uint8-array = [ 12 34 ];
    string = string_value: "foo bar baz";
    string-array = "foo", "bar", "baz";
    enum-int = <200>;
    enum-string = "whatever";
  };
};
```

Now we simply need to extend our bindings file by adding an entry for each of the node's properties in the _properties_ key:

`dts/bindings/custom-props-basics.yaml`
```yaml
description: Custom properties
compatible: "custom-props-basics"

properties:
  existent-boolean:
    type: boolean
  int:
    type: int
    required: true
  array:
    type: array
  uint8-array:
    type: uint8-array
  string:
    type: string
  string-array:
    type: string-array
  enum-int:
    type: int
    enum:
      - 100
      - 200
      - 300
  enum-string:
    type: string
    enum:
      - "whatever"
      - "works"
```

After recompiling ...

```bash
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 --build-dir ../build -- \
  -DEXTRA_DTC_OVERLAY_FILE="dts/playground/props-basics.overlay"
```

... we can now find macros for all of our properties in `devicetree_generated.h` for the node `/node_with_props`, and the comment block also indicates which binding was selected to generate the macros.

`build/zephyr/include/generated/devicetree_generated.h`
```c
/*
 * Devicetree node: /node_with_props
 *
 * Node identifier: DT_N_S_node_with_props
 *
 * Binding (compatible = custom-props-basics):
 *   /path/to/dts/bindings/custom-props-basics.yaml
 */

/* Node's full path: */
#define DT_N_S_node_with_props_PATH "/node_with_props"
/* --snip-- */
/* Generic property macros: */
/* --snip-- */
```

Let's have a look at the generated macros listed beyond the marker _Generic property macros_.

#### `boolean`

For the property `existent-boolean` of type `boolean`, the devicetree generator produces the following macros:

```c
#define DT_N_S_node_with_props_P_existent_boolean 1
#define DT_N_S_node_with_props_P_existent_boolean_EXISTS 1
```

Using what we've learned in the section about [understanding devicetree macro names](#understanding-devicetree-macro-names), we can easily understand the basename `DT_N_S_node_with_props_P_existent_boolean` of the generated macros:

`DT` is the devicetree prefix, `N` indicates that what follows is a node's path, `S` is a forward slash `/`, and finally `P` indicates the start of a property. Thus `DT_N_S_node_with_props_P_existent_boolean` essentially translates to `node=/node_with_props`, `property=existent_boolean`.

Since the `existent-boolean` property is present in the node in our overlay, its value translates to `0`. If we'd _remove_ the property from our node, we'd end up with the following:

```c
#define DT_N_S_node_with_props_P_existent_boolean 0
#define DT_N_S_node_with_props_P_existent_boolean_EXISTS 1
```

Thus, the value of *boolean*s is `0` if the property is _false_, or `1` if it is _true_.

What about `_EXISTS`? Remember that any path or property name is transformed to its _lowercase_ form in the devicetree macros. `_EXIST` is all uppercase, which indicates that it isn't a value definition, but a macro that is generated for use with the [devicetree API](#zephyrs-devicetree-api).

For properties of any type but `boolean`s, Zephyr's devicetree generator creates a matching `_EXISTS` macro _only_ if the property _exists_ in the devicetree. If a property is not present, no macros are generated. *Boolean*s are an exception where this macro is _always_ generated, since a missing property value means that the property is set to _false_.

> **Note:** In case you're wondering if it is possible to _unset_ or delete a boolean that is defined somewhere else in the devicetree - yes it is, and we'll try it out in the section about[deleting properties](#deleting-properties).

#### `int`

For the property `int` of type `int` in our node `/node_with_props`, Zephyr's devicetree generator produces the following macros:

```c
#define DT_N_S_node_with_props_P_int 1
#define DT_N_S_node_with_props_P_int_EXISTS 1
```

Unsurprisingly, for properties of type `int` the value of the property's macro is an _integer literal_. As mentioned in the [previous section](#boolean), the macro `_EXISTS` is created for every property that _exists_ in the devicetree. If we'd try to remove the property, however, we'd get the following error when rebuilding since we specified `int` as a _required_ property in our binding:

```
...
-- Found devicetree overlay: dts/playground/props-basics.overlay
devicetree error: 'int' is marked as required in 'properties:' in /path/to/dts/bindings/custom-props-basics.yaml, but does not appear in <Node /node_with_props in '/opt/nordic/ncs/v2.4.0/zephyr/misc/empty_file.c'>
```

If we'd remove `required: true` from the binding file _and_ delete the node's property in the overlay, both macros would indeed be removed from `devicetree_generated.h`; any search for `N_S_node_with_props_P_int` would fail.

> **Note:** Knowing how macros work in `C`, you might be curious why Zephyr _removes_ the `_EXISTS` macro instead of defining its value to _0_. After all, without using the compile time switches `#ifdef` or `#if defined()` you can't check a macro's value if the macro is not defined - or can you? Turns out there is a neat trick that allows to do this, and Zephyr makes use of it, but we won't go into detail about this trick in the section on [macrobatics](#macrobatics). If you still want to know how this works, have a look at the documentation of the `IS_ENABLED` macro in `zephyr/include/zephyr/sys/util_macro.h` and the macros it expands to. It is explained nicely in the macro documentation!

> **Note:** In case you specify the value of a node's property of type `int` in the devicetree in the _hexadecimal_ format, at the time of writing the integer literal is converted to its _decimal_ value in `devicetree_generated.h`.

#### `array` and `uint8-array`

For properties of the type `array` and `uint8-array`, the devicetree generator produces _initializer expressions_ in braces, whose elements are integer literals. In the [devicetree API section](#zephyrs-devicetree-api) we'll use those expressions as, well, initialization values for our variables or constants. The macro with the suffix `_LEN` defines the number of elements in the array.

For each element and its position _n_ within the array, the generator also produces the macros `_IDX_n` and `_IDX_n_EXISTS`. In addition, several `_FOREACH` macros (hidden in the below snippet) are generated that expand an expression for each element in the array _at compile time_.

The following macros are produced for our `array` property of type `array`:

```c
#define DT_N_S_node_with_props_P_array {10 /* 0xa */, 11 /* 0xb */, 12 /* 0xc */}
#define DT_N_S_node_with_props_P_array_IDX_0 10
#define DT_N_S_node_with_props_P_array_IDX_0_EXISTS 1
#define DT_N_S_node_with_props_P_array_IDX_1 11
#define DT_N_S_node_with_props_P_array_IDX_1_EXISTS 1
#define DT_N_S_node_with_props_P_array_IDX_2 12
#define DT_N_S_node_with_props_P_array_IDX_2_EXISTS 1
/* array_FOREACH_ ... */
#define DT_N_S_node_with_props_P_array_LEN 3
#define DT_N_S_node_with_props_P_array_EXISTS 1
```

For the node's `uint8-array` property with the corresponding type, a similar set of macros is generated. Only the `_FOREACH` macros are slightly different, but that doesn't concern us right now (in case you're still curious, check out the documentation of `DT_FOREACH_PROP_ELEM(` in the [Zephyr's devicetree API documentation][zephyr-dts-api]).

```c
#define DT_N_S_node_with_props_P_uint8_array {18 /* 0x12 */, 52 /* 0x34 */}
#define DT_N_S_node_with_props_P_uint8_array_IDX_0 18
#define DT_N_S_node_with_props_P_uint8_array_IDX_0_EXISTS 1
#define DT_N_S_node_with_props_P_uint8_array_IDX_1 52
#define DT_N_S_node_with_props_P_uint8_array_IDX_1_EXISTS 1
/* --snip-- uint8_array_FOREACH_ ... */
#define DT_N_S_node_with_props_P_uint8_array_LEN 2
#define DT_N_S_node_with_props_P_uint8_array_EXISTS 1
```

Just like for our `int` property, if we'd remove the `array` or `uint8-array` property from our node in the devicetree, no macros (not even the `_EXISTS` macros) are generated for the property.

#### `string`

For properties of the type `string`, the devicetree generator produces _string literals_. The generator also produces macros with a special suffix:

- `_STRING_UNQUOTED` contains the string literals without quotes and thus all values as _tokens_.
- `_STRING_TOKEN` produces a single token out of the string literals. Special characters and spaces are replaced by underscores.
- `_STRING_UPPER_TOKEN` produces the same token as `_STRING_TOKEN`, but in uppercase letters.

In addition, the generator also produces `_FOREACH` macros, which expand for each _character_ in the string. E.g., for our value "foo bar baz" with the string length _11_, the `_FOREACH` macro would expand _11_ times.

The following is a snipped of our `devicetree_generated.h` for the property `string`:

```c
#define DT_N_S_node_with_props_P_string "foo bar baz"
#define DT_N_S_node_with_props_P_string_STRING_UNQUOTED foo bar baz
#define DT_N_S_node_with_props_P_string_STRING_TOKEN foo_bar_baz
#define DT_N_S_node_with_props_P_string_STRING_UPPER_TOKEN FOO_BAR_BAZ
/* --snip-- string_FOREACH_ ... */
#define DT_N_S_node_with_props_P_string_EXISTS 1
```

> **Note:** The characters used by the token in `_STRING_TOKEN` are not converted to lowercase. If we'd use the value _"Foo Bar Baz"_ for `string`, the generated token would be `Foo_Bar_Baz`. Only the `_STRING_UPPER_TOKEN` is always all uppercase.

The string value as token can be useful, e.g., when using the token to form a `C` variable or code. It is very rarely used, though (in case you're curious, try looking for `DT_INST_STRING_TOKEN` in the Zephyr repository).

No macros are generated in case the property does not exist in the devicetree.

#### `string-array`

`string-array`s are handled similar to `array` and `uint8-array`: Instead of _integer_ literals, the devicetree generator produces _initializer expressions_ in braces whose elements are _string_ literals. The macro with the suffix `_LEN` defines the number of elements in the array.

For each element and its position _n_ within the array, the generator also produces the macros `_IDX_n` like we've seen for properties of type `string` - except that the generator won't produce `_FOREACH` macros for the characters within each string literal. Instead, several `_FOREACH` macros (hidden in the below snippet) are generated that expand an expression for each element in the array _at compile time_.

The following macros are produced for our `string-array` property of the same-named type:

```c
#define DT_N_S_node_with_props_P_string_array {"foo", "bar", "baz"}
#define DT_N_S_node_with_props_P_string_array_IDX_0 "foo"
#define DT_N_S_node_with_props_P_string_array_IDX_0_STRING_UNQUOTED foo
#define DT_N_S_node_with_props_P_string_array_IDX_0_STRING_TOKEN foo
#define DT_N_S_node_with_props_P_string_array_IDX_0_STRING_UPPER_TOKEN FOO
#define DT_N_S_node_with_props_P_string_array_IDX_0_EXISTS 1
/* --snip-- the same IDX_n_ macros are generated for "bar" and "baz" */
/* --snip-- string_array_FOREACH_ ... */
#define DT_N_S_node_with_props_P_string_array_LEN 3
#define DT_N_S_node_with_props_P_string_array_EXISTS 1
```

No macros are generated in case the property does not exist in the devicetree.

#### `enum`

For enumerations, the generator produces the same macros as it would for the corresponding base type, e.g., `int` or `string`, and in addition it generates `_ENUM` macros that indicate the position within th enumeration (and some more for `string`s).

For our enumeration `enum-int` with the allowed values _100_, _200_, and _300_, the generator produced the following macros for the selected value _200_:

```c
#define DT_N_S_node_with_props_P_enum_int 200
#define DT_N_S_node_with_props_P_enum_int_ENUM_IDX 1
#define DT_N_S_node_with_props_P_enum_int_EXISTS 1
```

Thus, in addition for the macros that would also have been generated if the property was a plain `int`, the generator produced an additional `_ENUM` macro indicating the index of the selected value within the enumeration.

The following is produced for `enum-string` with the allowed values _whatever_ and _works_, for the selected value _whatever_:

```c
#define DT_N_S_node_with_props_P_enum_string "whatever"
#define DT_N_S_node_with_props_P_enum_string_STRING_UNQUOTED whatever
#define DT_N_S_node_with_props_P_enum_string_STRING_TOKEN whatever
#define DT_N_S_node_with_props_P_enum_string_STRING_UPPER_TOKEN WHATEVER
#define DT_N_S_node_with_props_P_enum_string_ENUM_IDX 0
#define DT_N_S_node_with_props_P_enum_string_ENUM_TOKEN whatever
#define DT_N_S_node_with_props_P_enum_string_ENUM_UPPER_TOKEN WHATEVER
/* --snip-- enum_string_FOREACH_ ... */
#define DT_N_S_node_with_props_P_enum_string_EXISTS 1
```

Here, the generator produced the same marcos that we'd get if `enum-string` was a plain `string` (including the character-based `_FOREACH` macros), and again we get the index of the chosen value within the enumeration. In addition, the generator produces `TOKEN` and `TOKEN_UPPER` macros with an `ENUM` prefix - to make the tokens accessible using the devicetree API macros for enumerations.

### Labels and paths

Before we move on to properties using `phandle` types, let's have a look at labels and paths. As we've seen in the previous chapter about the devicetree basics, the [devicetree specification][devicetree-spec] allows _labels_ not only to refer to _nodes_ but also at other positions in the devicetree, e.g., in front of property _values_. In our example, three labels:

- One label `label_with_props` for our node `/node_with_props`,
- Two labels `second_value` and `string_value` for some property values.

`dts/playground/props-basics.overlay`
```dts
/ {
  label_with_props: node_with_props {
    compatible = "custom-props-basics";
    /* ... */
    array = <1 second_value: 2 3>;
    /* ... */
    string = string_value: "foo bar baz";
    string-array = "foo", "bar", "baz";
    /* ... */
  };
};
```

Zephyr's DTS generator accepts this input and - as we've seen - won't complain about the labels `second_value` and `string_value` refering to the property values. However, Zephyr simply ignores any label that doesn't refer to a _node_ and therefore doesn't generate anything usable for the property labels. Only the _node_ label `label_with_props` has a macro:

```bash
$ grep second_value ../build/zephyr/include/generated/devicetree_generated.h
$ grep string_value ../build/zephyr/include/generated/devicetree_generated.h
$ grep label_with_props ../build/zephyr/include/generated/devicetree_generated.h
#define DT_N_NODELABEL_label_with_props DT_N_S_node_with_props
```

The following macros are generated to represent a node and its labels:

`build/zephyr/include/generated/devicetree_generated.h`
```c
/*
 * Devicetree node: /node_with_props
 *
 * Node identifier: DT_N_S_node_with_props
 * --snip--
 */
/* Node's full path: */
#define DT_N_S_node_with_props_PATH "/node_with_props"
/* Node's name with unit-address: */
#define DT_N_S_node_with_props_FULL_NAME "node_with_props"
// --snip--
/* Existence and alternate IDs: */
#define DT_N_S_node_with_props_EXISTS 1
#define DT_N_INST_0_custom_props_basics DT_N_S_node_with_props
#define DT_N_NODELABEL_label_with_props DT_N_S_node_with_props
```

The _identifier_ that is used for our `/node_with_props` is `DT_N_S_node_with_props`, which is exactly what we'd expect for the given path and from what we've learned about [understanding devicetree macro names](#understanding-devicetree-macro-names). This _identifier_ is itself just a _token_: No macro with the name `DT_N_S_node_with_props` actually exists!

_Identifiers_ are used by the macros of the [devicetree API](#zephyrs-devicetree-api). As we'll see, these macros do nothing but paste together different _tokens_. E.g., accessing a node's property's value, all we need to do is paste together tokens:

- Using the node identifier `DT_N_S_node_with_props`,
- and knowing that `_P_` is used for property names,
- we end up with `DT_N_S_node_with_props_P_int` for the property `int`.

In turn, a _label_ is nothing else than a different name for our node's identifier, as we can see in the above snippet of `devicetree_generated.h`. Labels use the format `DT_N_NODELABEL_<label_in_lowercase>` as macro name and the node's identifier as value.

In addition, the generator also produces macros with the suffix `_PATH` and `FULL_NAME` that contain the node's full path and name including the unit address as string literals.

#### `aliases` and `chosen`

In the previous chapter about devicetree basics we've also seen the standard nodes `aliases` and `chosen`, predefined by the [devicetree specification][devicetree-spec], which allow referring to a node using a _phandle_ (either using a reference to its label or the `${/full/path}` syntax), or the node's full path as a string. Let's try this out in our overlay file:

`dts/playground/props-basics.overlay`
```dts
/ {
  aliases {
    alias-by-label = &label_with_props;
    alias-by-path = &{/node_with_props};
    alias-as-string = "/node_with_props";
  };

  chosen {
    chosen-by-label = &label_with_props;
    chosen-by-path = &{/node_with_props};
    chosen-as-string = "/node_with_props";
  };
  /* ... previous content ... */
};
```

The node's aliases are translated to macros with the format `DT_N_ALIAS_<alias_in_lowercase>` in the node's section _"Existence and alternate IDs"_ that we've seen just before. Looking at the generated ouput, it is now also clear why there's no clear distinction between the terms _"references"_ and _"phandle"_ in Zephyr:

```c
/* Existence and alternate IDs: */
#define DT_N_S_node_with_props_EXISTS 1
#define DT_N_ALIAS_alias_by_label       DT_N_S_node_with_props
#define DT_N_ALIAS_alias_by_path        DT_N_S_node_with_props
#define DT_N_ALIAS_alias_as_string      DT_N_S_node_with_props
#define DT_N_INST_0_custom_props_basics DT_N_S_node_with_props
#define DT_N_NODELABEL_label_with_props DT_N_S_node_with_props
```

Whether we reference a node using its label, a path reference or its path as string, it all translates to the node's _identifier_ token. The only difference is [which devicetree API](#zephyrs-devicetree-api) we use to obtain this token, as we'll see later.

Finally, `/chosen` nodes are listed in their own output section in `devicetree_generated.h`. The only difference to `/aliases` is, that each macro also gets its own `_EXISTS` pair:

```c
/*
 * Chosen nodes
 */
// --snip--
#define DT_CHOSEN_chosen_by_label                DT_N_S_node_with_props
#define DT_CHOSEN_chosen_by_label_EXISTS         1
#define DT_CHOSEN_chosen_by_path                 DT_N_S_node_with_props
#define DT_CHOSEN_chosen_by_path_EXISTS          1
#define DT_CHOSEN_chosen_as_string               DT_N_S_node_with_props
#define DT_CHOSEN_chosen_as_string_EXISTS        1
```

### Phandles

Having seen labels and the `/chosen` and `/aliases` nodes, we can guess what Zephyr's output for the types `phandle` and `phandles` is - maybe even `path` and `phandle-array` is. Nevertheless, we'll walk through the types using a similar example of what we've already seen in the previous chapter about the devicetree basics.

#### `path`, `phandle` and `phandles`

Let's get started by creating two new files `custom-props-phandles.yaml` and `props-phandles.overlay`:

```bash
$ tree --charset=utf-8 --dirsfirst.
├── dts
│   ├── bindings
│   │   ├── custom-props-basics.yaml
│   │   └── custom-props-phandles.yaml
│   └── playground
│       ├── props-basics.overlay
│       └── props-phandles.overlay
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

Our devicetree overlay is pretty much the same as we've seen in the previous chapter about devicetree basics, but for now we're skipping `phandle-array` properties. Thus, we're creating properties of type `path`, `phandle`, and `phandles`:

```dts
/ {
  label_a: node_a { /* Empty. */ };
  label_b: node_b { /* Empty. */ };

  node_refs {
    compatible = "custom-props-phandles";

    path-by-path = &{/node_a};
    path-by-label = &label_a;

    phandle-by-path = <&{/node_a}>;
    phandle-by-label = <&label_a>;

    // Simple array of phandles
    phandles = <&{/node_a} &label_b>;
  };
};
```

The matching bindings file has the following contents:

```yaml
description: Custom properties
compatible: "custom-props-phandles"

properties:
  path-by-path:
    type: path
  path-by-label:
    type: path
  phandle-by-path:
    type: phandle
  phandle-by-label:
    type: phandle
  phandles:
    type: phandles
```

In the build we could just switch out our overlay and ignore `props-basics.overlay`, but let's include both of them instead, remembering that we only need to separate the paths by semicolon:

```bash
$ rm -rf ../build
$ west build --board nrf52840dk_nrf52840 --build-dir ../build -- \
  -DDTC_OVERLAY_FILE="dts/playground/props-basics.overlay;dts/playground/props-phandles.overlay"
```

The build should succeed without any problems and both overlays should be detected. In the `devicetree_generated.h` we now find all of our new nodes, but we're especially interested in `node_refs` using our new binding `custom-props-phandles.yaml`:

```c
/*
 * Devicetree node: /node_refs
 *
 * Node identifier: DT_N_S_node_refs
 *
 * Binding (compatible = custom-props-phandles):
 *   /path/to/dts/bindings/custom-props-phandles.yaml
 */
// --snip--
/* Generic property macros: */
// --snip--
```

The properties `path-by-path` and `path-by-label` both have the type `path`. We might therefore expect a similar output that we got when specifying paths in the `/chosen` node, namely node identifiers. However, the only output we get is the following:

```c
#define DT_N_S_node_refs_P_path_by_path_EXISTS 1
#define DT_N_S_node_refs_P_path_by_label_EXISTS 1
```

So basically, the `type: path` exists in Zephyr, but at the time of writing does not produce any output - except for the `_EXISTS` macros, which are useless without any additional macros. This may seem surprising, but makes a lot of sense recalling that Zephyr's devicetree is not compiled into a devicetree blob `dtb`, but is instead used or resolved at _compile time_. Paths are of no use since nodes exist only as _identifiers_ and therefore tokens that are used to access a node's properties. At the time of writing, there is in fact not a single binding with in Zephyr that uses a `path`.

Let's move on to the generated output for the properties `phandle-by-path` and `phandle-by-label`:

```c
#define DT_N_S_node_refs_P_phandle_by_path DT_N_S_node_a
#define DT_N_S_node_refs_P_phandle_by_path_IDX_0 DT_N_S_node_a
#define DT_N_S_node_refs_P_phandle_by_path_IDX_0_PH DT_N_S_node_a
#define DT_N_S_node_refs_P_phandle_by_path_IDX_0_EXISTS 1
#define DT_N_S_node_refs_P_phandle_by_path_LEN 1
#define DT_N_S_node_refs_P_phandle_by_path_EXISTS 1

#define DT_N_S_node_refs_P_phandle_by_label DT_N_S_node_a
#define DT_N_S_node_refs_P_phandle_by_label_IDX_0 DT_N_S_node_a
#define DT_N_S_node_refs_P_phandle_by_label_IDX_0_PH DT_N_S_node_a
#define DT_N_S_node_refs_P_phandle_by_label_IDX_0_EXISTS 1
#define DT_N_S_node_refs_P_phandle_by_label_LEN 1
#define DT_N_S_node_refs_P_phandle_by_label_EXISTS 1
```

Zephyr's generator translated either property value into the node's identifier, just like we've seen for the `/aliases` and `/chosen` nodes. The format in which the property's value is provided in the devicetree does not matter for the generated output - which is of the same type `phandle`.

It is a bit surprising, though, that Zephyr also generated macros containing `_IDX_0` and the macro with the suffix `_LEN`: `phandle` typed properties only support a single value (try it out!). The reason for this becomes clear when looking at the output for our property `phandles` of the same-named type:

```c
#define DT_N_S_node_refs_P_phandles_IDX_0 DT_N_S_node_a
#define DT_N_S_node_refs_P_phandles_IDX_0_PH DT_N_S_node_a
#define DT_N_S_node_refs_P_phandles_IDX_0_EXISTS 1
#define DT_N_S_node_refs_P_phandles_IDX_1 DT_N_S_node_b
#define DT_N_S_node_refs_P_phandles_IDX_1_PH DT_N_S_node_b
#define DT_N_S_node_refs_P_phandles_IDX_1_EXISTS 1
/* --snip-- phandles_FOREACH */
#define DT_N_S_node_refs_P_phandles_LEN 2
#define DT_N_S_node_refs_P_phandles_EXISTS 1
```

Thus, the generated output for `phandle` and `phandles` is essentially handled in the same way. The only difference is, that for `phandles` _only_ the `_IDX_n` macros are generated and thus the _identifiers_ can only be accessed by index, whereas for the `phandle` type it is possible to retrieve the node's identifier using the node identifer and property name. We'll see this when we access _phandles_ via [the devicetree API](#zephyrs-devicetree-api).

In contrast to `array`, `uint8-array`, and `string-array`, no _initializer expression_ is generated - _phandles_ are not values but only tokens and thus cannot be assigned to any variable.

In summary: `path`s practically don't exist in Zephyr, `phandle` and `phandles` essentially produce the same output, and all devicetree references and paths translate to node _identifiers_ - which are just _tokens_.

#### `phandle-array`

Let's briefly recall why we need a `phandle-array`: In the previous chapter about the devicetree basics we've seen how `gpios` are represented by the nRF52840's devicetree (for a detailed explanation, please read the corresponding section in the previous chapter):

##### `phandle-array` in Zephyr

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```dts
/ {
  soc {
    gpio0: gpio@50000000 {
      compatible = "nordic,nrf-gpio";
      #gpio-cells = <2>;
      port = <0>;
    };

    gpio1: gpio@50000300 {
      compatible = "nordic,nrf-gpio";
      #gpio-cells = <2>;
      port = <1>;
    };
  };
};
```

Each instance handles _all_ pins in the corresponding port. However, some other devicetree node, e.g., an LED, needs a way to control and configure a single pin of a given port. We can't do this using a normal _phandle_, e.g., `<&gpio0>`, we need a mechanisim to pass some _parameters_ or _metadata_ along with the _phandle_. `phandle-array`s implement this exact usecase, allowing to pass a predefined number of cells with the _phandle_, e.g., as we can see in the nRF52840's development kit DTS file for `/leds/led_0`:

`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
/ {
  leds {
    led0: led_0 {
      gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
    };
  };
};
```

What we haven't seen yet, is how we know that the first cell _13_ is the pin within `gpio0`, and the second cell _GPIO_ACTIVE_LOW_ are configuration flags. This is part of the _semantics_ defined by the _binding_ of the _gpio_ nodes that are compatible with `nordic,nrf-gpio`:

`zephyr/dts/bindings/gpio/nordic,nrf-gpio.yaml`
```yaml
description: NRF5 GPIO node
compatible: "nordic,nrf-gpio"
# --snip--

gpio-cells:
  - pin
  - flags
```

Here we see that `gpio-cells` consists of two items `pin` and `flags`, which are matched exactly against the provided metadata in the given order. Thus, _13_ refers to the `pin` and `GPIO_ACTIVE_LOW` to its `flags`.

You may have noticed that the `gpio` nodes do not conform to the naming convention that we've seen in the last chapter. This makes it a bit awkward to explain what needs to be provided for a `phandle-array` and the referenced nodes. Therefore, we'll build our own from scratch.

##### A `phandle-array` from scratch

Let's also briefly review the rules for using `phandle-array`s. There's no need to understand them out of context, we'll see all of them in action in our example:

- By convention, a `phandle-array` property is plural and its name must thus end in _s_.
- The value of a `phandle-array` property is an array of phandles, but each phandle is followed by the a pre-defined number of cells for each referenced node.
- The number of cells that can follow a node's reference is specified by the node's _specifier cells_ property `#<prefix>-cells`.
- _Specifier cells_ have a defined naming convention: The name is formed by removing the plural '_s_' and attaching '_-cells_' to the name of the `phandle-array` property.

We'll now extend our `props-phandles.overlay` with a new property `phandle-array-of-refs`. Within this property, we'll reference `/node_a` and pass along two cells, and `/node_b` with just one cell as metadata:

`dts/playground/props-phandles.overlay`
```dts
/ {
  label_a: node_a { /* Empty. */ };
  label_b: node_b { /* Empty. */ };
  node_refs {
    compatible = "custom-props-phandles";
    /* --snip-- */
    phandle-array-of-refs = <&{/node_a} 1 2 &label_b 1>;
  };
};
```

Within our binding for `custom-props-phandles`, we can now add the property `phandle-array-of-refs` of type `phandle-array`:

`dts/bindings/custom-props-phandles.yaml`
```yaml
description: Custom properties
compatible: "custom-props-phandles"

properties:
  # --snip--
  phandle-array-of-refs:
    type: phandle-array
```

Here we fulfill our first rule, namely that our property name ends with an 's'. If we'd violate this rule, e.g., by using `phandle-array-of-ref` as property name, the devicetree compiler would reject the binding with the following error:

```
-- Found devicetree overlay: dts/playground/props-phandles.overlay
devicetree error: 'phandle-array-of-ref' in 'properties:' in /path/to/dts/bindings/custom-props-phandles.yaml has type 'phandle-array' and its name does not end in 's', but no 'specifier-space' was provided.
```

The mentioned _specifier-space_ is a way to work around the plural 's' rule and is used for properties that would otherwise result in weird names. We won't explain the workings of _specifier-space_ in this chapter, so I'll leave you with a reference [Zephyr's documentation on specifier-space][zephr-dts-bindings-specifier-space] in case you want to know how this works in detail.

For now, let's revert the change and use `phandle-array-of-refs` as property name and try to compile the project (the command didn't change). Now the devicetree compiler complains that our _specifier-cells_ are missing:

```
-- Found devicetree overlay: dts/playground/props-phandles.overlay
devicetree error: <Node /node_a in '/opt/nordic/ncs/v2.4.0/zephyr/misc/empty_file.c'> lacks #phandle-array-of-ref-cells
```

Looking at our value assignment `phandle-array-of-refs = <&{/node_a} 1 2 &label_b 1>;`, this error makes perfect sense, since the compiler has no way of knowing how may _cells_ are supposed to follow each node reference. We can update our overlay with the required properties for each referenced node:

`dts/playground/props-phandles.overlay`
```dts
/ {
  label_a: node_a { #phandle-array-of-ref-cells = <2>; };
  label_b: node_b { #phandle-array-of-ref-cells = <1>; };
  node_refs {
    /* --snip-- */
    phandle-array-of-refs = <&{/node_a} 1 2 &label_b 1>;
  };
};
```

Now the devicetree compiler knows, that any reference to `/node_a` in a property of type `phandle-array` must be followed by exactly _2_ cells, whereas a reference to `/node_b` only expects one cell. Does this fix do the trick? Let's try to recompile the example:

```
-- Found devicetree overlay: dts/playground/props-phandles.overlay
devicetree error: phandle-array-of-ref controller <Node /node_a in '/opt/nordic/ncs/v2.4.0/zephyr/misc/empty_file.c'> for <Node /node_refs in '/opt/nordic/ncs/v2.4.0/zephyr/misc/empty_file.c'> lacks binding
```

For `phandle-array`s, Zephyr really requires a binding since it otherwise isn't able to generate the required macros. Without the bindings for our referenced nodes, Zephyr _could_ have been able to generate a partial set of macros, but thankfully it was decided that such incomplete bindings are rejected entirely.

Since the number of required cells in our nodes is different, we need _two separate_ bindings. We'll again use the binding name as filename and create two files `dts/bindings/custom-cells-a.yaml` and `dts/bindings/custom-cells-b.yaml`:

```bash
$ tree --charset=utf-8 --dirsfirst.
├── dts
│   ├── bindings
│   │   ├── custom-cells-a.yaml
│   │   ├── custom-cells-b.yaml
│   │   ├── custom-props-basics.yaml
│   │   └── custom-props-phandles.yaml
│   └── playground
│       ├── props-basics.overlay
│       └── props-phandles.overlay
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

Within the binding files, as usual we declare compatibility using `compatible` key. We might expect that `#phandle-array-of-ref-cells` are placed under the _properties_ key in the binding. They are, however, [top level keys][zephyr-dts-bindings-top-level] and give a [_name_][zephyr-dts-bindings-specifier-names] to each cell that follows the reference:

`dts/bindings/custom-cells-a.yaml`
```yaml
description: Dummy for matching "cells"
compatible: "custom-cells-a"

phandle-array-of-ref-cells:
  - name-of-cell-one
  - name-of-cell-two
```

`dts/bindings/custom-cells-a.yaml`
```yaml
description: Dummy for matching "cells"
compatible: "custom-cells-b"

phandle-array-of-ref-cells:
  - name-of-cell-one
```

The above binding thus defines a _name_ and **semantics** for each value that follows the corresponding _phandle_ when used in a `phandle-array`. E.g., for `phandle-array-of-refs = <&{/node_a} 1 2 &label_b 1>;`, we now know that:
- For `/node_a`, the value _1_ is assigned to a cell named `name-of-cell-one` and the value _2_ to a cell named `name-of-cell-two`;
- for `/node_a`, the cell `name-of-cell-one` is assigned the value _1_.

We're therefore providing the means for the [devicetree API](#zephyrs-devicetree-api) for accessing the _metadata_ passed with the _phandle_ using a descriptive name.

> **Note:** One thing that is not immediately clear is, that number of elements in the `-cells` key of the _binding_ **must** match the number of `-cells = <n>` specified in the _devicetree_. It is, e.g., **not** possible to provide _more_ elements binding's list and restrict the exact number of cells using the matching devicetree property. Thus, for any node that has a binding, the property `-cells` in the devicetree is redundant.

We can now go ahead and assign the bindings to our nodes with the `compatible` property, and recompile the project.

`dts/playground/props-phandles.overlay`
```dts
/ {
  label_a: node_a {
    compatible = "custom-cells-a";
    #phandle-array-of-ref-cells = <2>;
  };
  label_b: node_b {
    compatible = "custom-cells-b";
    #phandle-array-of-ref-cells = <1>;
  };
  node_refs {
    compatible = "custom-props-phandles";
    /* --snip-- */
    phandle-array-of-refs = <&{/node_a} 1 2 &label_b 1>;
  };
};
```

For our node `/node_refs`, Zephyr's generator produces the following macros:

```c
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_0_EXISTS 1
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_0_PH DT_N_S_node_a
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_0_VAL_name_of_cell_one 1
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_0_VAL_name_of_cell_one_EXISTS 1
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_0_VAL_name_of_cell_two 2
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_0_VAL_name_of_cell_two_EXISTS 1
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_1_EXISTS 1
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_1_PH DT_N_S_node_b
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_1_VAL_name_of_cell_one 1
#define DT_N_S_node_refs_P_phandle_array_of_refs_IDX_1_VAL_name_of_cell_one_EXISTS 1
/* --snip-- phandle_array_of_refs_FOREACH */
#define DT_N_S_node_refs_P_phandle_array_of_refs_LEN 2
#define DT_N_S_node_refs_P_phandle_array_of_refs_EXISTS 1
```

Unsurprisingly, the output looks a lot like what we receive for `phandles`: We get a set of macros for each index _n_ `_IDX_n`, but now the length _n_ `_LEN` is not the total number of _cells_ within the array, but matches the number sets of _phandles and metadata_.

Since each element is no longer a plain _phandle_, a macro with the suffix `_PH` is generated for the _phandle_ and thus node's identifier, and `_VAL_x` macros are generated for each cell that follows the _phandle_; the specifier name _x_ matches the name provided for each specifier in the binding.

Due to the fact that the index within a `phandle-array` does not refer to a cell's value, values for `phandle-array` properties are sometimes assigned using the alternative syntax that we've already seen in the previous chapter:
- `phandle-array-of-refs = <&{/node_a} 1 2>, <&label_b 1>;` is used instead of
- `phandle-array-of-refs = <&{/node_a} 1 2 &label_b 1>;`.

The generated output is identical.

### Deleting properties

In our `props-basics.overlay` example we've seen the boolean property `existent-boolean`. We've learned that a boolean is set to _true_ if it exists in the devicetree, otherwise it is _false_. Let's assume that a boolean property exists in some include file, how could you set this property to _false_ in your overlay?

We can try this in our existing `dts/playground/props-basics.overlay`, where we `node_with_props` has `existent-boolean` in its properties and thus sets its value to _true_. Without modifying `node_with_props` directly, we can _delete_ the property after the node's declaration using `/delete-property/`. We'll do this for both, `existent-boolean` and `string`, as follows:

`dts/playground/props-basics.overlay`
```dts
/ {
  label_with_props: node_with_props {
    compatible = "custom-props-basics";
    existent-boolean;
    /* --snip-- */
  };
};

&label_with_props {
  /delete-property/ existent-boolean;
  /delete-property/ string;
}
```

This leads to the same generated output as if the properties were not defined in the node at all. As we've seen, for `boolean`s the property's value is simply set to _0_. For the property `string`, no more macros are generated.

```c
#define DT_N_S_node_with_props_P_existent_boolean 0
#define DT_N_S_node_with_props_P_existent_boolean_EXISTS 1
/* No macros DT_N_S_node_with_props_P_string ... */
```

### Full example

Instead of providing the full example in text, I'll leave you with a TODO: link to the example application on Github. The overlays and bindings have the same name and content as used throughout the previous sections.


## Zephyr's devicetree API

Now that we've seen both, DTS files and their bindings, we'll wrap up this chapter by looking at how the generated macros are accessed in an application. We'll cover the most common macros from `zephyr/include/zephyr/devicetree.h`. Those macros are the basis for most Zephyr modules, e.g., `gpio`s, that in turn provide their own devicetree API built on top of the mentioned macros.

> **Note:** For details, you'll always need to refer to the [official devicetree API documentation][zephyr-dts-api] and to the corresponding subsystems. The documentation is pretty amazing and has lots of examples. We'll cover the basics here so that you can easily navigate the documentation.

### Macrobatics

Having seen the macros provided by Zephyr's generator script, it's pretty obvious how nodes and property values are accessed by Zephyr's devicetree API: The big "mistery" behind the devicetree API is simply _token pasting_. In its simplest form, tokens are pasted or concatenated using the token-pasting operator `##`:

```c
// Let's assume we use macros with the format <identifier>_<property>,
// e.g., a "node" with property "string_value" set to "123-test"
#define node_string_value "123-test"

// Tokens in C macros are pasted using `##`.
#define TOKEN_CAT_3(a1, a2, a3)      a1##a2##a3
#define NODE_PROP(node_id, property) TOKEN_CAT_3(node, _, string_value)

// The following resolves to `node_string_value` and thus to "123-test"
static const char *str = NODE_PROP(NODE, string_value);
```

Zephyr takes token pasting to another level in its devicetree API, using variadic macro arguments, macro repetitions, and other neat tricks. In his presentation ["Zephyr Devicetree Mysteries, Solved"][zephyr-summit-22-devicetree] at the _Zephyr Development Summit 2022_, Bolivar fittingly describes those mechanisms as "macrobatics".

I highly recommend watching [his presentation][zephyr-summit-22-devicetree] (maybe after reading through this chapter), and if you're curious, digging into `util_macro.h` and `util_loops.h` in `zephyr/include/zephyr/sys`. The files are very well documented, and the macros `MACRO_MAP_CAT` (used by the `DT_PATH`) and `IS_ENABLED` (resolving the `_EXISTS` macros) are especially interesting.

### Node identifiers

Let's review how we can reference a node in the devicetree using our `props-basics.overlay`:

`dts/playground/props-basics.overlay`
```dts
/ {
  aliases {
    alias-by-label = &label_with_props;
    alias-by-path = &{/node_with_props};
    alias-as-string = "/node_with_props";
  };

  chosen {
    chosen-by-label = &label_with_props;
    chosen-by-path = &{/node_with_props};
    chosen-as-string = "/node_with_props";
  };

  label_with_props: node_with_props {
    /* ... */
  };
};
```

> **Note:** We're deliberatly skipping references to a node using `phandle` properties since those are _properties_, and properties are accessed differently, as we'll see in the next section (yes, technically node references in `/chosen` and `/aliases` are properties too, but those nodes are predefined by the [devicetree specification][devicetree-spec]).

Having thoroughly explored devicetree, we know that we have the following possibilities to refer to a node:
- Using the `/aliases` or `/chosen` nodes,
- using a globally unique _node label_,
- or via the node's full _path_.

As we've seen in the previous chapter about the devicetree basics, it is up to you to decide what you want to use in your application. It typically might make sense to use `/aliases` or `/chosen` nodes, since paths and even node labels can be different for, e.g., different boards.

How can we reference nodes? We know that nodes as such do not exist in Zephyr's generated devicetree: Nodes are translated into _tokens_ and do not have any value on their own. In Zephyr these tokens are referred to as _node identifiers_ and they are used as parameters by other macros, e.g., to retrieve a property value. Such tokens are practically just prefixes for property macros.

Enough talk. Let's start with `/aliases`: Zephyr provides the `DT_ALIAS` macro get a node's identifier via its alias:

```c
// Node identifier by /aliases node.
// DT_ALIAS(alias) = DT_N_ALIAS_ ## alias
#define NODE_PROPS_ALIAS_BY_LABEL   DT_ALIAS(alias_by_label)
#define NODE_PROPS_ALIAS_BY_PATH    DT_ALIAS(alias_by_path)
#define NODE_PROPS_ALIAS_BY_STRING  DT_ALIAS(alias_as_string)
```

The `DT_ALIAS` macro simply pastes `DT_N_ALIAS_` and the given identifier and therefore resolves to the macros that we've already seen in the section about [`aliases` and `chosen`](#aliases-and-chosen). In turn, all alias macros resolve to the node identifier `DT_N_S_node_with_props`.

| API                         | Pasted macro                 | Node identifier          |
| :-------------------------- | :--------------------------- | :----------------------- |
| `DT_ALIAS(alias_by_label)`  | `DT_N_ALIAS_alias_by_label`  | `DT_N_S_node_with_props` |
| `DT_ALIAS(alias_by_path)`   | `DT_N_ALIAS_alias_by_path`   | `DT_N_S_node_with_props` |
| `DT_ALIAS(alias_as_string)` | `DT_N_ALIAS_alias_as_string` | `DT_N_S_node_with_props` |

You might have noticed that the properties in the DTS file use _dashes_, e.g., "`alias-by-label`", whereas in the code we need to replace the dashes by _underscores_ for the alias name, resulting in `alias_by_label`. We've learned this transformation in the section about [understanding devicetree macro names](#understanding-devicetree-macro-names). The macros in the API, however, cannot change the format of their parameters and thus identifiers. Therefore, you have to perform the _"lowercase-and-underscores"_ transformation for all identifiers passed to devicetree API macros:

- All letters are converted to lowercase,
- and non-alphanumerics characters are converted to underscores "`_`".

No other transformations are needed, e.g., you'll never need to provide a path with `/` replaced by `_S_` or prefix devicetree macros yourself. The devicetree API does that for you and provides macros for __all__ such usecases.

As mentioned before, the resulting _node identifer_ `DT_N_S_node_with_props` is only a _token_. No macro with the same name exists, the following search does not yield any results:

```bash
$ grep -E 'DT_N_S_node_with_props[ \t]' \
  ../build/zephyr/include/generated/devicetree_generated.h
$
```

Therefore we cannot use a node identifier on its own anywhere in the code. Well, since it is a _token_, technically we could use it, e.g., as variable name, but that is not the point. The point is, the `C` preprocessor cannot further resolve the macro, and wherever you'd use the identifier `DT_N_S_node_with_props`, it would be placed as such text (not string literal) in code.

Moving on! Just like for `/aliases`, Zephyr provides the `DT_CHOSEN` macro get a node's identifier via the `/chosen` node:

```c
// Node identifier by /chosen node.
// DT_CHOSEN(chosen) = DT_N_CHOSEN_ ## chosen
#define NODE_PROPS_CHOSEN_BY_LABEL  DT_CHOSEN(chosen_by_label)
#define NODE_PROPS_CHOSEN_BY_PATH   DT_CHOSEN(chosen_by_path)
#define NODE_PROPS_CHOSEN_AS_STRING DT_CHOSEN(chosen_as_string)
```

The `DT_CHOSEN` macro simply pastes `DT_N_CHOSEN_` and the given identifier and again resolves to the macros that we've seen in the section about [`aliases` and `chosen`](#aliases-and-chosen). Just like the aliases, all chosen nodes resolve to the node identifier `DT_N_S_node_with_props`:

| API                           | Pasted macro                   | Node identifier          |
| :---------------------------- | :----------------------------- | :----------------------- |
| `DT_CHOSEN(chosen_by_label)`  | `DT_N_CHOSEN_chosen_by_label`  | `DT_N_S_node_with_props` |
| `DT_CHOSEN(chosen_by_path)`   | `DT_N_CHOSEN_chosen_by_path`   | `DT_N_S_node_with_props` |
| `DT_CHOSEN(chosen_as_string)` | `DT_N_CHOSEN_chosen_as_string` | `DT_N_S_node_with_props` |

This leaves us with node labels and retrieving a node's identifier by its path. Zephyr provides the two macros `DT_NODELABEL` and `DT_PATH` for this:

```c
// Node identifier by label.
// DT_NODELABEL(label) = DT_N_NODELABEL_ ## label
#define NODE_PROPS_BY_LABEL DT_NODELABEL(label_with_props)

// Node identifier by path.
// DT_PATH(...) = DT_N_ pasted with S_<node> for all nodes in the path.
#define NODE_PROPS_BY_PATH  DT_PATH(node_with_props)
```

The `DT_NODELABEL` is again a simple token pasting of `DT_N_NODELABEL_` and the provided _label_, resulting in the macro `DT_N_NODELABEL_label_with_props` that we've seen in the section about [labels and paths](#labels-and-paths), which again resolves to the node identifier `DT_N_S_node_with_props`.

The macro `DT_PATH` is a _variadic_ macro which allows retrieving a node's idenitifier using its full path. The path to a node is specified by the sequence of nodes, starting at the root node `/`. Each argument is thus a _node identifier_; the root node `/` is omitted. The devicetree API simply pastes `DT_N` and `_S_<node>` for each node (in "lowercase-and-underscores" form) in the path, resulting in the _identifier_ `DT_N_S_node_with_props`.

> **Note:** The path `/soc/uart@40002000` is an example for a node with several levels, obtained using `DT_PATH(soc, uart_40002000)`. Notice that it is **not** possible to use node labels in paths.

### Property values




from devicetree to C structure -> done via DT_ macros, e.g., GPIO_DT_SPEC_GET
TODO: could we create our own little DT_SPEC_GET at least for prop-basic?

### Macrobatics


## Practice run

### `status`

### Remapping `uart0`

### Switching boards

## Summary

## Further reading

[nordicsemi]: https://www.nordicsemi.com/
[nordicsemi-academy-devicetree]: https://academy.nordicsemi.com/topic/devicetree/ters
[devicetree-spec]: https://www.devicetree.org/specifications/
[linux-dts-bindings]: https://docs.kernel.org/devicetree/bindings/writing-schema.html
[yaml]: https://yaml.org/

[zephyr-build-board-revision]: https://docs.zephyrproject.org/latest/develop/application/index.html#application-board-version
[zephyr-dts-overlays]: https://docs.zephyrproject.org/latest/build/dts/howtos.html#set-devicetree-overlays
[zephyr-dts-intro-input-and-output]: https://docs.zephyrproject.org/latest/build/dts/intro-input-output.html
[zephyr-dts-bindings-intro]: https://docs.zephyrproject.org/latest/build/dts/bindings-intro.html
[zephyr-dts-bindings-syntax]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html
[zephyr-dts-bindings-syntax-include]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#include
[zephyr-dts-bindings-syntax-properties]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#properties
[zephyr-dts-bindings-syntax-bus]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#bus
[zephyr-dts-api]: https://docs.zephyrproject.org/latest/build/dts/api/api.html

<!--
[zephyr-kconfig]: https://docs.zephyrproject.org/latest/build/kconfig/index.html#configuration-system-kconfig
[zephyr-dts]: https://docs.zephyrproject.org/latest/build/dts/index.html
[zephyr-dts-howto]: https://docs.zephyrproject.org/latest/build/dts/howtos.html
[zephyr-dts-bindings-api]: https://docs.zephyrproject.org/latest/build/dts/api/bindings.html
[zephyr-dts-bindings-types]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#type
[zephyr-dts-intro-bindings-properties]: https://docs.zephyrproject.org/latest/build/dts/intro-syntax-structure.html#properties
[zephyr-dts-intro-input-and-output]: https://docs.zephyrproject.org/latest/build/dts/intro-input-output.html
[zephyr-dts-intro-property-values]: https://docs.zephyrproject.org/latest/build/dts/intro-syntax-structure.html#writing-property-values
[zephyr-dts-phandles]: https://docs.zephyrproject.org/latest/build/dts/phandles.html

[zephyr-dts-bindings-location]: https://docs.zephyrproject.org/latest/build/dts/bindings-intro.html#where-bindings-are-located
[zephr-dts-bindings-specifier-cells]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#specifier-cell-names-cells
[zephyr-dts-api-chosen]: https://docs.zephyrproject.org/latest/build/dts/api/api.html#devicetree-chosen-nodes
[zephyr-kconfig]: https://docs.zephyrproject.org/latest/build/kconfig/index.html#configuration-system-kconfig
[zephyr-summit-22-devicetree]: https://www.youtube.com/watch?v=w8GgP3h0M8M&list=PLzRQULb6-ipFDwFONbHu-Qb305hJR7ICe -->
