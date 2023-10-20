
- [Goals](#goals)
- [Prerequisites](#prerequisites)
- [Warm-up](#warm-up)
- [Devicetree overlays](#devicetree-overlays)
  - [Automatic overlays](#automatic-overlays)
  - [Files by example](#files-by-example)
- [Towards bindings](#towards-bindings)
  - [Extending the example application](#extending-the-example-application)
- [](#)
- [Zephyr's devicetree API](#zephyrs-devicetree-api)
- [Summary](#summary)
- [Further reading](#further-reading)

## Goals

TODO: rename to devicetree_bindings?

## Prerequisites

This builds up on devicetree basics

<!-- TODO: or "easy pieces" -->
## Warm-up

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

### Files by example

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









##

Devicetree nodes are matched to bindings using their compatible properties.

## Zephyr's devicetree API

`lowercase-and-underscores` form (!!) due to macros
`zephyr/include/zephyr/devicetree.h`

```
/* Node's full path: */
#define DT_N_S_node_with_phandle_PATH "/node_with_phandle"

/* Node parent (/) identifier: */
#define DT_N_S_node_with_phandle_PARENT DT_N
```

`#define DT_N_NODELABEL_<>`
there is no DT_P_LABEL (no property or value label), only DT_NODELABEL
`#define DT_N_ALIAS_<>`

`/* Generic property macros: */`
`#define DT_N_S_<node>_P_`

from devicetree to C structure -> done via DT_ macros, e.g., GPIO_DT_SPEC_GET
TODO: could we create our own little DT_SPEC_GET at least for prop-basic?

## Summary

## Further reading

[nordicsemi]: https://www.nordicsemi.com/
[nordicsemi-academy-devicetree]: https://academy.nordicsemi.com/topic/devicetree/ters
[devicetree-spec]: https://www.devicetree.org/specifications/

[zephyr-build-board-revision]: https://docs.zephyrproject.org/latest/develop/application/index.html#application-board-version
[zephyr-dts-overlays]: https://docs.zephyrproject.org/latest/build/dts/howtos.html#set-devicetree-overlays

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
