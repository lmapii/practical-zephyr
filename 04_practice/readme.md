
# Practice

This [Zephyr freestanding application][zephyr-app-freestanding] is by the fifth article of the "Practical Zephyr" series. This modified *Blinky* application explores Zephyr's use of the devicetree API.

The application can be built for [Nordic's nRF52840 development kit](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk) using the following command:

```bash
west build --board nrf52840dk_nrf52840
```

The goal of this application is to demonstrate advanced devicetree concepts (refer to the article).

> **Note:** The [`tasks.py`](./tasks.py) script is used by the [GitHub action](../.github/workflows/ci.yml) for building this application using [invoke](https://www.pyinvoke.org/).
