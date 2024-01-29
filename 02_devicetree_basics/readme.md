
# Devicetree from first principles

This [Zephyr freestanding application](https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-freestanding-app) is used in the third article of the "Practical Zephyr" series. The application itself is just a dummy, the important files are the devicetree overlay files to demonstrate the basic devicetree types used by Zephyr:

- [`props-basics.overlay`](./dts/playground/props-basics.overlay) shows all basic types except for _phandles_. It is based on [Zephyr's tests for Node.props](https://github.com/zephyrproject-rtos/zephyr/blob/main/scripts/dts/python-devicetree/tests/test.dts#L349)
- [`props-phandles.overlay`](./dts/playground/props-phandles.overlay) shows the _phandle_, _phandles_, and _phandle-array_ types.

The following command can be used to build the application for [Nordic's nRF52840 development kit](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk):

```bash
west build --board nrf52840dk_nrf52840 --
    -DEXTRA_DTC_OVERLAY_FILE="dts/playground/props-phandles.overlay;dts/playground/props-basics.overlay"
```

The goal of this application is only demonstrating the basic devicetree syntax and types: No code is generated for the corresponding nodes, the only output is `build/zephyr/zephyr.dts`. The next demo application shows devicetree _bindings_, which leads to the generation of the corresponding macros.

> **Note:** The [`tasks.py`](./tasks.py) script is used by the [GitHub action](../.github/workflows/ci.yml) for building this application using [invoke](https://www.pyinvoke.org/).
