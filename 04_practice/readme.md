
In the previous articles, we covered _devicetree_ in great detail: We've seen how we can create our own nodes, we've seen the supported property types, we know what bindings are, and we've seen how to access the devicetree using Zephyr's `devicetree.h` API. With this article, we'll finally look at a _practical_ example.

With this pratical example we'll see that our deep dive into devicetree was really only covering the basics that we need to understand more advanced concepts, such as [Pin Control][zephyr-pinctrl]. Finally, we'll run the practical example on two boards with different MCUs - showing maybe the main benefit of using Zephyr.

TODO: extend for Kconfig if done.

- [Prerequisites](#prerequisites)
- [Warm up](#warm-up)
- [Devicetree with `gpio`](#devicetree-with-gpio)
  - [Blinky with a `/chosen` LED node](#blinky-with-a-chosen-led-node)
- [Dissecting the GPIO devicetree specification](#dissecting-the-gpio-devicetree-specification)
  - [Applying the devicetree API](#applying-the-devicetree-api)
  - [Reviewing *phandle-array*s](#reviewing-phandle-arrays)
  - [Device objects and driver compatibility](#device-objects-and-driver-compatibility)
    - [Macrobatics: Resolving device objects with `DEVICE_DT_GET`](#macrobatics-resolving-device-objects-with-device_dt_get)
    - [Macrobatics: Declaring compatilble drivers and device object](#macrobatics-declaring-compatilble-drivers-and-device-object)
  - [Summary](#summary)
- [The `status` property](#the-status-property)
  - [Intermezzo: Power profiling](#intermezzo-power-profiling)
- [Devicetree with `UART`](#devicetree-with-uart)
  - [`pinctrl` - remapping `uart0`](#pinctrl---remapping-uart0)
- [Kconfig with `memfault`](#kconfig-with-memfault)
- [Switching boards](#switching-boards)
- [Conclusion](#conclusion)
- [Further reading](#further-reading)

## Prerequisites

This article is part of an article _series_. In case you haven't read the previous articles, please go ahead and have a look. This article requires that you're able to build and flash a Zephyr application to the board of your choice, and that you're familiar with _devicetree_.

We'll be mainly using the [development kit for the nRF52840][nordicsemi-nrf52840-dk] and will run the example on an [STM32 Nucleo-64 development board][stm-nucleo] towards the end of the article, but you can follow along with any target - real or virtual.

> **Note:** A full example application including all files that we'll see throughout this article is available in the [`04_practice` folder of the accompanying GitHub repository](https://github.com/lmapii/practical-zephyr/tree/main/04_practice).



## Warm up

If you've been following along, you should know the drill by now: Let's create a new freestanding application with the files listed below:

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

The `prj.conf` remains empty for now, and the `CMakeLists.txt` only includes the necessary boilerplate to create a Zephyr application with a single `main.c` source file. As an application, we'll a `main` function that simply puts the MCU back to sleep at fixed intervals:

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

As usual, I'll build the application for my [nRF52840 Development Kit from Nordic][nordicsemi], but you can use any of [Zephyr's long list of supported boards][zephyr-boards] - or an emulation target.

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build
```



## Devicetree with `gpio`

Let's start with a classic: The blinking LED. The easiest way to control a low-power LED is using a GPIO, and one obvious way to approach this problem, is to jump straight to the [`gpio` API][zephyr-api-gpio] documentation. In contrast to [Zephyr's OS service documentation][zephyr-os-services], however, the documentation for [peripherals][zephyr-peripherals] is typically restricted to the API, without further explaining the subsystem.

Instead, Zephyr's chosen approach is to provide a list of related **code samples** for each subsystem, showing the API in action. And Zephyr comes with _hundreds_ of [samples and demos][zephyr-samples-and-demos]! In the list of related samples for the `gpio` subsystem, we also find the good old [blinky example][zephyr-samples-blinky].

> **Note:** [Blinky][zephyr-samples-blinky] is an example for a [Zephyr _repository_ application][zephyr-app-repository]. We continue using a [_freestanding_ application][zephyr-app-freestanding], and in the next article we'll finally create a [_workspace_ application][zephyr-app-workspace].

Straight from the documentation, we follow the _"Open in GitHub"_ link to find the application in Zephyr's repository, and simply copy the contents from [`main.c`][zephyr-samples-blinky-main] into our own application. We'll now compare the `gpio` devicetree API with the "plain" API in `zephyr/include/zephyr/devicetree.h` that we've seen in the last article.


### Blinky with a `/chosen` LED node

The *Blinky* example choses the LED devicetree node using the _alias_ `led0`. Zephyr keeps its devicetrees clean and we've seen that aliases, including `led0` are usually consistent throughout supported boards. Thus, if there's a board supported by Zephyr that has at least one light on it that works like LED, you can be sure that there's an `led0` alias for it:

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
  aliases {
    led0 = &led0;
  };
};
```

Since we're practicing, however, we'll use a `/chosen` node called `app-led` instead. We add this node using a new board overlay file `boards/nrf52840dk_nrf52840.overlay` for the nRF52840 development kit:

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── boards
│   └── nrf52840dk_nrf52840.overlay
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

`boards/nrf52840dk_nrf52840.overlay`
```dts
/ {
    chosen {
        app-led = &led0;
    };
};
```

In the example application, we use `DT_CHOSEN(app_led)` instead of `DT_ALIAS(led0)`, but that's about the only change we need. Notice that again we specify the chosen node's name using its "lowercase-and-underscore" form `app_led` instead of the node's name `app-led` in the devicetree. With a few minor adaptions our application looks as follows:

```c
/** \file main.c */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define SLEEP_TIME_MS 1000U
#define LED_NODE      DT_CHOSEN(app_led)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

void main(void)
{
    int err = 0;

    if (!gpio_is_ready_dt(&led)) { return; }

    err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (err < 0) { return; }

    while (1)
    {
        (void) gpio_pin_toggle_dt(&led);
        k_msleep(SLEEP_TIME_MS);
    }
}
```

Building and flashing the application we indeed end up with a blinking LED.

```bash
$ west build --board nrf52840dk_nrf52840 --build-dir ../build
$ west flash --build-dir ../build
```

TODO: GIF
TODO: Doesn't a blinking LED doesn't put a smile on your face?



## Dissecting the GPIO devicetree specification

Looking through the function prototypes of `gpio_is_ready_dt`, `gpio_pin_configure_dt`, and `gpio_pin_toggle_dt`,  you'll notice that they take a "_GPIO specification_" `const struct gpio_dt_spec *spec` as parameter. We find the matching declaration in Zephyr's `gpio.h`:

`zephyr/include/zephyr/drivers/gpio.h`
```c
struct gpio_dt_spec {
  /** GPIO device controlling the pin */
  const struct device *port;
  /** The pin's number on the device */
  gpio_pin_t pin;
  /** The pin's configuration flags as specified in devicetree */
  gpio_dt_flags_t dt_flags;
};
```


### Applying the devicetree API

Let's ignore the structure's contents for now. Instead, let's use `GPIO_DT_SPEC_GET` and see it compares with what we've learned about the devicetree API in the last articles.

So far, we've only used the macros from the devicetree API `zephyr/include/zephyr/devicetree.h`. Now we see how Zephyr's drivers create their own devicetree macros on top of this basic API. We can find the macro declaration in Zephyr's `gpio.h` header file:

`zephyr/include/zephyr/drivers/gpio.h`
```c
#define GPIO_DT_SPEC_GET(node_id, prop) GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, 0)
#define GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)                 \
  {                                                                 \
    .port = DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(node_id, prop, idx)), \
    .pin = DT_GPIO_PIN_BY_IDX(node_id, prop, idx),                  \
    .dt_flags = DT_GPIO_FLAGS_BY_IDX(node_id, prop, idx),           \
  }
```

Just like we've seen in the last article, Zephyr uses devicetree macros to create an initializer expression for the matching type that should be used with the corresponding macro. The macros `DT_GPIO_PIN_BY_IDX` and `DT_GPIO_FLAGS_BY_IDX` simply expand to the `DT_PHA_<x>` macros we've already used when accessing `phandle-array`s:

```c
#define DT_GPIO_PIN_BY_IDX(node_id, gpio_pha, idx) \
  DT_PHA_BY_IDX(node_id, gpio_pha, idx, pin)
#define DT_GPIO_FLAGS_BY_IDX(node_id, gpio_pha, idx) \
  DT_PHA_BY_IDX_OR(node_id, gpio_pha, idx, flags, 0)
```

Ignoring the `.port` field (we'll get to that, don't worry), `GPIO_DT_SPEC_GET` expands to a very familiar format, where `.pin` and `.dt_flags` are initialized using the `pin` and `flags` _specifier cells_ from our devicetree:

```c
#define GPIO_DT_SPEC_GET(node_id, prop)                              \
  {                                                                  \
    .port = DEVICE_DT_GET(/* --snip-- */),                           \
    .pin = DT_PHA_BY_IDX(node_id, prop, /*idx*/0, pin),              \
    .dt_flags = DT_PHA_BY_IDX_OR(node_id, prop, /*idx*/0, flags, 0), \
  }
```

> **Note:** The downside of using C macros is that, well, they're macros. Macros are expanded by the preprocessor, and the preprocessor doesn't care about types. It therefore also doesn't care if you use `GPIO_DT_SPEC_GET` as initializer for an incompatible type. This _may_ only fail during compile time in case the assigned variable or constant doesn't have a compatible type.


### Reviewing *phandle-array*s

Properties of type `phandle-array` are heavily used in devicetrees. Since we have one at hand with our *Blinky* example, let's use it to review what we've learned about `phandle-array`s thus far. The `/leds/led0` is defined in the board's DTS file as follows:

`(reduced) zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```dts
/ {
  leds {
    compatible = "gpio-leds";
    led0: led_0 {
      gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
    };
  };
};
```

The matching binding specifies that all child nodes of `led` have the required property `gpios` of type `phandle-array`:

`(reduced) zephyr/dts/bindings/led/gpio-leds.yaml`
```yaml
compatible: "gpio-leds"
child-binding:
  description: GPIO LED child node
  properties:
    gpios:
      type: phandle-array
      required: true
```

Nothing unexpected here: Since `gpios` is of type `phandle-array`, we can use it to refer to any of the board's GPIO instances. Looking at the above binding you might ask yourself, except for the property's name, how do we know which nodes we can refer to in this `phandle-array`?

Without any knowledge about the _compatible driver_, you cannot know that.

Looking at the *devicetree* in isolation, you can, in fact, use references to _any_ node, as long as the node has the matching `#gpio-cells` property. You could, however, create your own node and binding, where `#gpio-cells` doesn't use `pin` and `flags` as specifiers. E.g., we could define our own binding `custom-cells-a`:

`dts/bindings/custom-cells-a.yaml`
```yaml
description: Dummy for matching "cells"
compatible: "custom-cells-a"
gpio-cells:
  - name-of-cell-one
  - name-of-cell-two
  - name-of-cell-three
```

In our board overlay, we can create a matching dummy `node_a`, and overwrite the property `gpio` of `led0` with a reference to this dummy node:

`boards/nrf52840dk_nrf52840.overlay`
```dts
/ {
    chosen {
        app-led = &led0;
    };
    node_a {
        compatible = "custom-cells-a";
        #gpio-cells = <3>;
    };
};
&led0 {
    gpios = <&{/node_a} 1 2 3>;
};
```

If we were to revert our `main.c` file to the original dummy application without any references to the `gpio` subsystem, the build passes without warnings and we'd find our `/leds/led_0` node with the following properties in the merged `zephyr.dts` file:

`build/zephyr/zephyr.dts`
```dts
/ {
  leds {
    compatible = "gpio-leds";
    led0: led_0 {
      gpios = < &{/node_a} 0x1 0x2 0x3 >;
      label = "Green LED 0";
    };
  };
};
```

There is no mechanism in *devicetree* that allows declaring a `phandle-array` that is _"generic over a type"_ or _"something GPIO compatible"_. E.g., there's no such thing as a _base class_, _interface_, or _trait_ that you might now from programming languages.

Even if there was an annotation `phandle-array<T>`, e.g., to specify the required `compatible` property, what would we provide for `T`? GPIO nodes use different models depending on the vendor, e.g., the following snippets show the `compatible` properties of the GPIO nodes of the nRF52840 and STM32:

`zephyr/dts/arm/nordic/nrf52840.dtsi`
```
/ {
  soc {
    gpio0: gpio@50000000 { compatible = "nordic,nrf-gpio"; };
    gpio1: gpio@50000300 { compatible = "nordic,nrf-gpio"; };
  };
};
```

`zephyr/dts/arm/st/c0/stm32c0.dtsi`
```dts
/ {
  soc {
    pinctrl: pin-controller@50000000 {
      gpioa: gpio@50000000 { compatible = "st,stm32-gpio"; };
      gpiob: gpio@50000400 { compatible = "st,stm32-gpio"; };
      gpioc: gpio@50000800 { compatible = "st,stm32-gpio"; };
      gpiof: gpio@50001400 { compatible = "st,stm32-gpio"; };
    };
  };
};
```

By discarding such a requirement, we gain flexibility - at the cost of stronger typing _within the devicetree_. If we were to try and use the Zephyr GPIO API with the above assignment to `gpios`, the application would fail to compile since the required _specifier cells_ do not have the names `pin` and `flags` read using the `DT_PHA_BY_IDX[_OR]` macros in the `GPIO_DT_SPEC_GET` macro:

```c
#define GPIO_DT_SPEC_GET(node_id, prop)                              \
  {                                                                  \
    .port = DEVICE_DT_GET(/* --snip-- */),                           \
    .pin = DT_PHA_BY_IDX(node_id, prop, /*idx*/0, pin),              \
    .dt_flags = DT_PHA_BY_IDX_OR(node_id, prop, /*idx*/0, flags, 0), \
  }
```

You might have noticed that the `GPIO_DT_SPEC_GET` macro uses the `_OR` variant of the phandle macro to read the `flags` from the devicetree:

* For nodes with a _compatible binding_ that have both, `pin` and `flags` specifiers, the devicetree compiler ensures that cells for both values exist whenever the node is references in a `phandle-array`; providing a value for `flags` is **not** optional for such phandles.
* For nodes with a _compatible binding_ that does **not** have the `flags` specifier, the value _0_ is used in the specification. The compatible driver can also completely ignore the _flags_ field in any function call.

This adds another level of flexibility to the generic binding `gpio-leds` for LEDs, supporing any kind of GPIO node LEDs, regardless of whether or not they support using `flags`, e.g., as follows:

```dts
/ {
  leds {
    compatible = "gpio-leds";
    led0: led_0 { gpios = <&gpio0 13>; };
  };
};
```


### Device objects and driver compatibility

We now know how the `.pin` and `.dt_flags` fields of the structure are populated, but what about the `.port`? Before we try to figure out what a "device" is, let's try to find out what the macro expands to.

```c
#define GPIO_DT_SPEC_GET(node_id, prop)                                  \
  {                                                                      \
    .port = DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(node_id, prop, /*idx*/0)), \
    .pin = DT_PHA_BY_IDX(node_id, prop, /*idx*/0, pin),                  \
    .dt_flags = DT_PHA_BY_IDX_OR(node_id, prop, /*idx*/0, flags, 0),     \
  }
```

> **Disclaimer:** This section is another nose dive into Zephyr's use of the devicetree and may be considered advanced (or unnecessarily detailed). In case you're happy with the information about the GPIO specification as it is and don't want to know about the `.port` assignment, skip ahead to the [next section about the `status` property](#the-status-property).

The macro parameters are given in our application and expand as described in the following snippet:

```c
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_CHOSEN(app_led), gpios);
// Knowing that `DT_CHOSEN(app_led)` expands to `DT_CHOSEN_app_led`,
// we can look up `DT_CHOSEN_app_led` in `devicetree_generated.h`,
// which is defined as `DT_N_S_leds_S_led_0`.

// Thus:
// - node_id = DT_N_S_leds_S_led_0
// - prop    = gpio
```

#### Macrobatics: Resolving device objects with `DEVICE_DT_GET`

Before resolving macros manually, it is always worth looking into the documentation. We have two nexted macros, so it makes sense to check `DT_GPIO_CTLR_BY_IDX` first. The API documentation claims, that it can be used to
"_get the node identifier for the controller phandle from a gpio phandle-array property_". For the assignment `gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;` in our LED node, we'd thus expect to get the node identifer for the phandle `&gpio0`.

Let's have a look at the macro and its expansion:

```c
// We want to know the expansion of the following macro:
#define DT_GPIO_CTLR_BY_IDX(node_id, gpio_pha, idx) \
  DT_PHANDLE_BY_IDX(node_id, gpio_pha, idx)

#define DT_PHANDLE_BY_IDX(node_id, prop, idx) \
  DT_CAT6(node_id, _P_, prop, _IDX_, idx, _PH)

// Given:
// node_id = DT_N_S_leds_S_led_0
// prop    = gpios
// idx     = 0

// DT_GPIO_CTLR_BY_IDX
//    = DT_N_S_leds_S_led_0 ## _P_ ## gpios ## _IDX_ ## 0 ## _PH
//    = DT_N_S_leds_S_led_0_P_gpios_IDX_0_PH
```

Thus, `DT_GPIO_CTLR_BY_IDX` resolves to `DT_N_S_leds_S_led_0_P_gpios_IDX_0_PH`. We can find a macro for that token in `devicetree_generated.h`, which indeed resolves to the GPIO node's identifier `DT_N_S_soc_S_gpio_50000000`:

`build/zephyr/include/generated/devicetree_generated.h`
```c
#define DT_N_S_leds_S_led_0_P_gpios_IDX_0_PH DT_N_S_soc_S_gpio_50000000
```

On we go! `DEVICE_DT_GET` takes this node identifier as its parameter `node_id`.

```c
// Original macro from zephyr/include/zephyr/device.h
#define DEVICE_DT_GET(node_id) (&DEVICE_DT_NAME_GET(node_id))
```

What can the documentation tell us about this macro?

> Returns a pointer to a device object created from a devicetree node, if any device was allocated by a driver. If no such device was allocated, this will fail at linker time. If you get an error that looks like `undefined reference to __device_dts_ord_<N>` [...]

Since we don't know yet what "_device objects_" are, this is a bit cryptic. There is, however, one good hint in the linker error message: It seems like this macro is trying to provide a reference to a symbol called `__device_dts_ord_<N>`, where `N` is a number.

Without any more _macrobatics_, your editor of choice should be able to resolve this macro for you. The following is a screenshot from `vscode`, where the last line indeed shows that - for my application - `.port` is assigned a reference to `__device_dts_ord_11`:

![Screenshot macrobatics](../assets/vscode-macrobatics.png?raw=true "macrobatics")

We could be satisfied with this resolution, but we're not, are we? Aren't you curious how we get to this reference and especially the ordinal _11_ by only passing the node identifer to this macro? Well, I am so it's time for some more macrobatics!

In the following snippet we replace referenced macros step by step _without_ showing their definition, and thus - to some extent - perform the same expansion that is performed by the preprocessor. Since the steps are not exactly the same, however, we're calling them "_replacements_" and not _"expansions"_:

```c
// Original macro from zephyr/include/zephyr/device.h
#define DEVICE_DT_GET(node_id) (&DEVICE_DT_NAME_GET(node_id))

// 1st replacement: Given DEVICE_DT_NAME_GET = DEVICE_NAME_GET(Z_DEVICE_DT_DEV_ID(node_id))
#define DEVICE_DT_GET_1st(node_id) (&DEVICE_NAME_GET(Z_DEVICE_DT_DEV_ID(node_id)))

// 2nd replacement: Given DEVICE_NAME_GET = _CONCAT(__device_, dev_id)
#define DEVICE_DT_GET_2nd(node_id) (&_CONCAT(__device_, ,Z_DEVICE_DT_DEV_ID(node_id)))

// 3rd replacement: Z_DEVICE_DT_DEV_ID = _CONCAT(dts_ord_, DT_DEP_ORD(node_id))
#define DEVICE_DT_GET_3rd(node_id) (&_CONCAT(__device_, _CONCAT(dts_ord_, DT_DEP_ORD(node_id))))

// 4th replacement: DT_DEP_ORD = DT_CAT(node_id, _ORD)
#define DEVICE_DT_GET_4th(node_id) (&_CONCAT(__device_, _CONCAT(dts_ord_, DT_CAT(node_id, _ORD))))
```

> **Note:** In case you're wondering what the difference between `_CONCAT` and `DT_CAT` is, experiment with token pasting. Hint: One of the macros simply pastes tokens, whereas the other one also expands the resulting token.

It's easy enough to see how we end up with the prefix `__device_dts_ord_`, but it is not entirely clear yet how we get to the ordinal _11_. For this, we still need to resolve `DT_CAT(node_id, _ORD)`. Remembering that we already resolved our input parameter `node_id` to `DT_N_S_soc_S_gpio_50000000`, `DT_CAT` simply pastes the two tokens, and we end up with `DT_N_S_soc_S_gpio_50000000_ORD`. But what's that? This is a macro that is provided by Zephyr's devicetree generator:

```bash
$ grep DT_N_S_soc_S_gpio_50000000_ORD ../build/zephyr/include/generated/devicetree_generated.h
#define DT_N_S_soc_S_gpio_50000000_ORD 11
```

In simple words, Zephyr's devicetree generator assigns an _ordinals_ to device instances. This is a fundamental of Zephyr's _device_ API `zephyr/include/zephyr/device.h`, which is used to represent devices and their instances. Since this is a rather elaborate concept that goes beyond the scope of this article series, I'll leave you with a link to Zephyr's official documentation about the [device driver model][zephyr-drivers] and [instance-based APIs][zephyr-dts-api-instance].

For now, it is enough to know that Zephyr creates symbols for each device instance in the devicetree. In fact, in `devicetree_generated.h` we can find a list of the node ordering and thus ordinals of each instance at the very beginning of the file. Here, we find the ordinal _11_ of the GPIO node `/soc/gpio@50000000`, and also the ordinal of the second GPIO node `/soc/gpio@50000300`:

`build/zephyr/include/generated/devicetree_generated.h`
```c
/*
 * Generated by gen_defines.py
 *
 * DTS input file:
 *   /path/to/build/zephyr/zephyr.dts.pre
 *
 * Directories with bindings:
 *   /opt/nordic/ncs/v2.4.0/nrf/dts/bindings, /path/to/dts/bindings, $ZEPHYR_BASE/dts/bindings
 *
 * Node dependency ordering (ordinal and path):
 *   0   /
 *   1   /aliases
 *   2   /analog-connector
 * --snip--
 *   11  /soc/gpio@50000000
 * --snip--
 *   104 /soc/gpio@50000300
 * --snip--
 */

// --snip--
#define DT_N_S_soc_S_gpio_50000300_ORD 104
// --snip--
#define DT_N_S_soc_S_gpio_50000000_ORD 11
```

Now we can finalize the macro expansion of `DEVICE_DT_GET`:

```c
// given `DT_N_S_soc_S_gpio_50000000` for `node_id`:
// DT_CAT(node_id, _ORD)) resolves to `DT_N_S_soc_S_gpio_50000000_ORD`

#define DEVICE_DT_GET_4th(node_id) (&_CONCAT(__device_, _CONCAT(dts_ord_, DT_CAT(node_id, _ORD))))
#define DEVICE_DT_GET_x_1st()      (&_CONCAT(__device_, _CONCAT(dts_ord_, DT_N_S_soc_S_gpio_50000000_ORD)))
#define DEVICE_DT_GET_x_2nd()      (&_CONCAT(__device_, dts_ord_11))
#define DEVICE_DT_GET_x_3rd()      (&__device_dts_ord_11)
```

With this, we finally pieced together the complete assignment for our LED's GPIO specification:

```c
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
// =  {
//    .port = (&__device_dts_ord_11),
//    .pin = 13 /* DT_N_S_leds_S_led_0_P_gpios_IDX_0_VAL_pin */,
//    .dt_flags = 1 /* DT_N_S_leds_S_led_0_P_gpios_IDX_0_VAL_flags */,
//  }
```

That's all good and well, but where is this symbol `__device_dts_ord_11`? With all this macro magic, we can imagine that it is very unlikely that a search for `__device` within Zephyr's codebase will provide any valuable hint about the symbol's location. Instead, we can use a much more reliable method: Let's look for the symbol in the `.map` file:

```bash
$ grep -sw __device_dts_ord_11 ../build/zephyr/zephyr.map -A 1 -B 3
 .z_device_PRE_KERNEL_140_
    0x0000000000006550  0x30 zephyr/drivers/gpio/libdrivers__gpio.a(gpio_nrfx.c.obj)
    0x0000000000006550           __device_dts_ord_104
    0x0000000000006568           __device_dts_ord_11
 .z_device_PRE_KERNEL_155_
```

> **Note:** Your `grep` mileage may vary, I've just added `-A 1` and `-B 3` since I know that, in my application, the corresponding object and all the remaining instances are visible if I include one line _after_ (`-A 1`) and three lines _before_ (`-B 3`) the occurrence.

We've found it! It seems to be declared in Nordic's GPIO driver `zephyr/drivers/gpio/gpio_nrfx.c`.


#### Macrobatics: Declaring compatilble drivers and device object

I promised that we won't go into detail about [Zephyr's device driver model][zephyr-drivers] - and we won't. In this section, we'll only look at how the device instances are defined and how the connection with the nodes in the devicetree is established.

The two important parts within `gpio_nrfx.c` are the following:

```c
#define DT_DRV_COMPAT nordic_nrf_gpio
/* --snip-- the entire driver implementation */

#define GPIO_NRF_DEVICE(id)                 \
  /* --snip-- */                            \
  DEVICE_DT_INST_DEFINE(id, gpio_nrfx_init, \
      /* --snip-- */                        \
      &gpio_nrfx_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_DEVICE)
```

The macro definition `DT_DRV_COMPAT` is placed at the beginning of the file. It is essentially the device driver equivalent to the `compatible` property of devicetree nodes and the same-named key in the devicetree binding: It is defined to the "lowercase-and-underscore" form `nordic_nrf_gpio` of `nordic,nrf-gpio`.

At the end of the file, the device driver creates instances for all GPIO devices with the `status` property set to "ok" using the macro `DT_INST_FOREACH_STATUS_OKAY`. And that's where somehow the "_device objects_" `__device_dts_ord_104` and `__device_dts_ord_11` of the corresponding GPIO nodes are created.

Let's dissect these macros one by one, starting with `DT_INST_FOREACH_STATUS_OKAY`, which is more concise but also more complex:

```c
#define DT_INST_FOREACH_STATUS_OKAY(fn)                       \
  COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT),       \
    (UTIL_CAT(DT_FOREACH_OKAY_INST_, DT_DRV_COMPAT)(fn)),     \
    ()                                                        \
  )
```

For the expansion of `COND_CODE_1` we rely on the docs, which state that this macro _"insert[s] code depending on whether _flag expands to 1 or not."_. The first parameter is the *_flag*, the second parameter the code in case *_flag* expands to 1, and finally, the third parameter is the code that is used in case the *_flag* is not 1.

To get to the flag, we need to check `DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)`:

```c
#define DT_HAS_COMPAT_STATUS_OKAY(compat) \
  IS_ENABLED(DT_CAT(DT_COMPAT_HAS_OKAY_, compat))
```

We've seen the `IS_ENABLED` macro already: It is a more intricate macro that allows evaluating tokens even if they are not even defined. Since we know that in our case `compat` is `nordic_nrf_gpio`, we're looking for a macro `DT_COMPAT_HAS_OKAY_nordic_nrf_gpio`, which, as usual, we can find in `devicetree_generated.h`:

```bash
grep -w DT_COMPAT_HAS_OKAY_nordic_nrf_gpio ../build/zephyr/include/generated/devicetree_generated.h
#define DT_COMPAT_HAS_OKAY_nordic_nrf_gpio 1
```

Zephyr's devicetree generator creates a lot more information than just macros for property values of our devicetree nodes! Here, we see that the generator also provides macros indicating whether it encountered some nodes that claim compatibility with "`nordic,nrf-gpio`". It does this for each and every driver and device. This allows heavy optimizations in the codebase as we've seen above.

Now we found out that our driver is used, and thus the second parameter of `COND_CODE_1` is expanded. We've already seen that `UTIL_CAT` concatenates and expands the provided macros, which are `DT_FOREACH_OKAY_INST_` and our `DT_DRV_COMPAT` macro, which expands to `nordic_nrf_gpio`, thus, we're now looking for a macro `DT_FOREACH_OKAY_INST_nordic_nrf_gpio`:

```bash
grep -w DT_FOREACH_OKAY_INST_nordic_nrf_gpio ../build/zephyr/include/generated/devicetree_generated.h
#define DT_FOREACH_OKAY_INST_nordic_nrf_gpio(fn) fn(0) fn(1)
```

Also here Zephyr's devicetree generator creates a macro for the matching expansion of device drivers based on the number of devices in the devicetree claiming compatibility with the `nordic,nrf-gpio` device driver. For each device it encounters it adds a `fn(n)` to the macro. Given this macro, we now know the complete expansion of the `DT_INST_FOREACH_STATUS_OKAY` macro:

```c
// `DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_DEVICE)`
// given DT_DRV_COMPAT = nordic_nrf_gpio
// expands to:
GPIO_NRF_DEVICE(0) GPIO_NRF_DEVICE(1)
```

The above macros are expanded for the corresponding device _instance_ number, but we still don't know how to associate the instance number with the _ordinals_ _11_ and _104_. So far, we can only establish a connection to the nodes `/soc/gpio@50000000` and `/soc/gpio@50000300` via the _ordinals_.

This leads us to the expansion of `DEVICE_DT_INST_DEFINE` used by `GPIO_NRF_DEVICE`, and its own expansion:

```c
#define GPIO_NRF_DEVICE(id)                 \
  /* --snip-- */                            \
  DEVICE_DT_INST_DEFINE(id, gpio_nrfx_init, \
      /* --snip-- */                        \
      &gpio_nrfx_drv_api_funcs);

// Where:
// #define DEVICE_DT_INST_DEFINE(inst, ...) \
//   DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)
#define GPIO_NRF_DEVICE_1st(id)                     \
  /* --snip-- */                                    \
  DEVICE_DT_DEFINE(DT_DRV_INST(id), gpio_nrfx_init, \
      /* --snip-- */                                \
      &gpio_nrfx_drv_api_funcs);
```

The documentation of `DT_DRV_INST` states that it is used to get the "_node identifier for an instance of a `DT_DRV_COMPAT` compatible_". But how do we get there; how do we match a node to a device instance number? Looks like we'll have to do a fast expansion for `DT_DRV_INST` as well:

```c
// Expansion of DT_DRV_INST, given
// #define DT_INST(inst, compat) UTIL_CAT(DT_N_INST, DT_DASH(inst, compat))
#define DT_DRV_INST_1st(inst)  DT_INST(inst, DT_DRV_COMPAT)
#define DT_DRV_INST_2nd(inst)  UTIL_CAT(DT_N_INST, DT_DASH(inst, DT_DRV_COMPAT))
#define DT_DRV_INST_3rd(inst)  UTIL_CAT(DT_N_INST, _ ## inst ## DT_DRV_COMPAT)

// Given:
// - DT_DRV_COMPAT = nordic_nrf_gpio
// - inst = 0
#define DT_DRV_INST_0_1st  UTIL_CAT(DT_N_INST, DT_DASH(0, nordic_nrf_gpio))
#define DT_DRV_INST_0_2nd  UTIL_CAT(DT_N_INST, _0_nordic_nrf_gpio)
#define DT_DRV_INST_0_3rd  DT_N_INST_0_nordic_nrf_gpio
```

```bash
$ grep -w DT_N_INST_0_nordic_nrf_gpio ../build/zephyr/include/generated/devicetree_generated.h
#define DT_N_INST_0_nordic_nrf_gpio DT_N_S_soc_S_gpio_50000000
$ grep -w DT_N_INST_1_nordic_nrf_gpio ../build/zephyr/include/generated/devicetree_generated.h
#define DT_N_INST_1_nordic_nrf_gpio DT_N_S_soc_S_gpio_50000300
```

Thus, `DT_DRV_INST` uses the generated macros to associate the node with the _instance_ number:
- `/soc/gpio@50000000` is instance _0_, and
- `/soc/gpio@50000300` is instance _1_ of the driver `nordic,nrf-gpio`.

This is it! We now have the connection from our _instance_ number to _node identifier_ and thus the devicetree node. We've already seen how we can get from the _node identifier_ to the _"device object's"_ name, and can therefore apply that knowledge to declare the symbol. And through several expansions, the device macro `DEVICE_DT_DEFINE` effectively does exactly that: It declares a global constant using the name `__device_dts_ord_<N>`, where `N` is the ordinal it obtains using the _node identifier_ of the corresponding instance.

The following is an oversimplified definition for the macro `DEVICE_DT_DEFINE`:

```c
// Simplified definition of DEVICE_DT_DEFINE for the device object:
#define DEVICE_DT_DEFINE_simplified(node_id, ...)                     \
  const struct device DEVICE_NAME_GET(Z_DEVICE_DT_DEV_ID(node_id)) =  \
    Z_DEVICE_INIT(/* --snip-- */)

// For node_id = DT_N_S_soc_S_gpio_50000000 ->
// const struct device __device_dts_ord_11 = ...
// For node_id = DT_N_S_soc_S_gpio_50000300 ->
// const struct device __device_dts_ord_104 = ...
```

In practice, Zephyr also creates other symbols that it uses to populate the device object, e.g., it also contains a function table for the entire GPIO API, such that a call to a `gpio` function leads to the correct call of the vendor's function. E.g., a call to `gpio_pin_toggle_dt` for a GPIO that is compatible with `nordic,nrf-gpio` eventually leads to a call to `gpio_nrfx_port_toggle_bits`.

For details, however, you'll need to refer to Zephyr's official documentation about the [device driver model][zephyr-drivers] and [instance-based APIs][zephyr-dts-api-instance], or have a look at the tutorial ["Mastering Zephyr Driver Development"][zephyr-ds-2022-driver-dev] by Gerard Marull Paretas from the Zephyr Development Summit 2022.


### Summary

We now not only know how the GPIO subsystem uses the macros from `devicetree.h` to get the _specifiers_ for `gpio` properties of type `phandle-array`, but we also know what the device objects are and how they're resolved:

```c
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
// =  {
//    .port = (&__device_dts_ord_11),
//    .pin = 13 /* DT_N_S_leds_S_led_0_P_gpios_IDX_0_VAL_pin */,
//    .dt_flags = 1 /* DT_N_S_leds_S_led_0_P_gpios_IDX_0_VAL_flags */,
//  }
```

Zephyr's devicetree generator provides much more than just macros for accessing property values in the devicetree. It also creates the necessary macros to _associate_ driver instances with nodes, and to _create_ the corresponding instances in the device driver itself. And then some ...





## The `status` property

`gpio_is_ready_dt` vs. `!DT_NODE_HAS_STATUS(LED_NODE, okay)`

### Intermezzo: Power profiling

I'm not associated with Nordic in any way, but I want to show the PPK.

Also, with over 10 years experience in the embedded industry and having seen MCUs of several vendors, the documentation and the software framework does not compare.


## Devicetree with `UART`

### `pinctrl` - remapping `uart0`

## Kconfig with `memfault`














finding conflicts ??

GPIO_DT_SPEC_GET -> easy, but device drivers are more complex, e.g., when trying to understand the is_ok
devicetree runtime API can be complex, macros are quite straight foward.


Blinky


GPIO API



```c
/** \file main.c */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED_NODE DT_CHOSEN(app_led)
#if !DT_NODE_EXISTS(LED_NODE)
#error "Missing /chosen node 'app-led'."
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

#if !DT_NODE_HAS_STATUS(LED_NODE, okay)
#error "LED node status is not okay."
#endif

#define SLEEP_TIME_MS 1000U

void main(void)
{
    int err   = 0;

    if (!gpio_is_ready_dt(&led))
    {
        return;
    }

    err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (err != 0)
    {
        return;
    }

    while (1)
    {
        (void) gpio_pin_toggle_dt(&led);
        k_msleep(SLEEP_TIME_MS);
    }
}
```

Kconfig: Memfault?
Devicetree: routing a gpio and rerouting uart.

nRF devicetree extension is nice, but it doesn't even show its own psels (e.g., UART) so the pinout only shows GPIOs


pinctrl

## Switching boards

switch to STM32

TODO: show that `gpio` is not generic by defining our own node and showing that it is _not_ compatible between boards.

`zephyr/boards/arm/nucleo_c031c6/nucleo_c031c6.dts`

even though boards cannot be "simply switched" and there'll always be some special usecases, the entire development environment is the same, the APIs are pretty much the same,

basically, Zephyr relieves us from having to create wrappers for our own application, and the toolchain remains the same.

## Conclusion

## Further reading






[nordicsemi]: https://www.nordicsemi.com/
[nordicsemi-dev-academy]: https://academy.nordicsemi.com/
[nordicsemi-nrf52840-dk]: https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk

[stm-nucleo]: https://www.st.com/en/evaluation-tools/nucleo-c031c6.html

[zephyr-samples-and-demos]: https://docs.zephyrproject.org/latest/samples/index.html
[zephyr-samples-blinky]: https://docs.zephyrproject.org/latest/samples/basic/blinky/README.html
[zephyr-samples-blinky-main]: https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/basic/blinky/src/main.c

[zephyr-api-gpio]: https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html#gpio-api

[zephyr-dts-api-instance]: https://docs.zephyrproject.org/latest/build/dts/api/api.html#devicetree-inst-apis

[zephyr-app-repository]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-repo-app
[zephyr-app-freestanding]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-freestanding-app
[zephyr-app-workspace]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-workspace-app

[zephyr-pinctrl]: https://docs.zephyrproject.org/latest/hardware/pinctrl/index.html
[zephyr-os-services]: https://docs.zephyrproject.org/latest/services/index.html
[zephyr-peripherals]: https://docs.zephyrproject.org/latest/hardware/peripherals/index.html
[zephyr-drivers]: https://docs.zephyrproject.org/latest/kernel/drivers/index.html

[zephyr-ds-2022-driver-dev]: https://www.youtube.com/watch?v=o-f2qCd2AXo