
# Exploring Kconfig

This is a [Zephyr freestanding application][zephyr-app-freestanding] used in the second article of the "Practical Zephyr" series. It shows the various possibilities of using `Kconfig` files.

Browse the files and read the comments for more details - or even better, follow along the article series!

**Plain west build for a specific board**

```bash
west build --board nrf52840dk_nrf52840
```
* [`prj.conf`](./prj.conf)
* [`boards/nrf52840dk_nrf52840.conf`](./boards/nrf52840dk_nrf52840.conf)

**Release build**

```bash
west build --board nrf52840dk_nrf52840 -- -DCONF_FILE=prj_release.conf
```
* [`prj_release.conf`](./prj.conf)
* [`boards/nrf52840dk_nrf52840_release.conf`](./boards/nrf52840dk_nrf52840.conf)

**Using extra `Kconfig` files**

```bash
west build --board nrf52840dk_nrf52840 -- -DEXTRA_CONF_FILE="extra0.conf;extra1.conf
```
* [`prj.conf`](./prj.conf)
* [`boards/nrf52840dk_nrf52840.conf`](./boards/nrf52840dk_nrf52840.conf)
* [`extra0.conf`](./extra0.conf)
* [`extra0.conf`](./extra1.conf)

**Release build with extra `Kconfig` files**

```bash
west build --board nrf52840dk_nrf52840 -- -DCONF_FILE="prj_release.conf" -DEXTRA_CONF_FILE="extra1.conf;extra0.conf
```
* [`prj_release.conf`](./prj.conf)
* [`boards/nrf52840dk_nrf52840_release.conf`](./boards/nrf52840dk_nrf52840.conf)
* [`extra0.conf`](./extra0.conf)
* [`extra0.conf`](./extra1.conf)

> **Note:** The [`tasks.py`](./tasks.py) script is used by the [GitHub action](../.github/workflows/ci.yml) for building this application using [invoke](https://www.pyinvoke.org/).
