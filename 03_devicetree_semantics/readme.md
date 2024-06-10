
# Devicetree semantics

This [Zephyr freestanding application](https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-freestanding-app) is used in the [fourth article](https://interrupt.memfault.com/blog/practical_zephyr_dt_semantics) of the "Practical Zephyr" series. The application itself is just a dummy but shows important Devicetree API macros. The most important files are the Devicetree overlay files and Devicetree **bindings**:

- [`props-basics.overlay`](./dts/playground/props-basics.overlay) shows all basic types except for phandles. The matching bindings can be found in [`custom-props-basics.yaml`](./dts/bindings/custom-props-basics.yaml)
- [`props-phandles.overlay`](./dts/playground/props-phandles.overlay) shows the _phandle_, _phandles_, and _phandle-array_ types. The matching bindings can be found in [`custom-props-phandles.yaml`](./dts/bindings/custom-props-phandles.yaml), and the bindings for the _specifier cells_ used in the phandle-array are defined in [`custom-cells-a.yaml`](./dts/bindings/custom-cells-a.yaml) and [`custom-cells-b.yaml`](./dts/bindings/custom-cells-b.yaml).

In addition, this application demonstrates three overlay files that are picked up automatically when building for [Nordic's nRF52840 development kit](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk):

- [`app.overlay`](./app.overlay)
- [`nrf52840dk_nrf52840.overlay`](./nrf52840dk_nrf52840.overlay)
- [`boards/nrf52840dk_nrf52840.overlay`](./boards/nrf52840dk_nrf52840.overlay)

The following command can be used to build the application:

```bash
west build --board nrf52840dk_nrf52840 --
    -DEXTRA_DTC_OVERLAY_FILE="dts/playground/props-phandles.overlay;dts/playground/props-basics.overlay"
```

The goal of this application is to demonstrate Devicetree _bindings_ and thus the macros produced by Zephyr's Devicetree generator script in `build/zephyr/include/generated/devicetree_generated.h`. The [`main.c`](./src/main.c) application shows the basic use of the Devicetree API macros.

> **Note:** The [`tasks.py`](./tasks.py) script is used by the [GitHub action](../.github/workflows/ci.yml) for building this application using [invoke](https://www.pyinvoke.org/).
