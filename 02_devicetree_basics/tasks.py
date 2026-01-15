"""
Invoke tasks for the 02_devicetree_basics demo.
This file exists mainly for simplifying the CI configuration.
"""

from invoke import task


def __runall__(c, msg, cmds):
    print(f"###\n###\n###\n### {msg}\n###")
    [c.run(cmd) for cmd in cmds]  # pylint: disable=expression-not-assigned


@task
def ci(c):
    __runall__(
        c,
        "Plain west build",
        [
            "rm -rf build",
            "west build --no-sysbuild --board nrf52840dk/nrf52840",
        ],
    )
    __runall__(
        c,
        "west build props-basics",
        [
            "rm -rf build",
            "west build --no-sysbuild --board nrf52840dk/nrf52840 -- "
            + "-DEXTRA_DTC_OVERLAY_FILE=dts/playground/props-basics.overlay",
        ],
    )
    __runall__(
        c,
        "west build props-phandles",
        [
            "rm -rf build",
            "west build --no-sysbuild --board nrf52840dk/nrf52840 -- "
            + "-DEXTRA_DTC_OVERLAY_FILE=dts/playground/props-phandles.overlay",
        ],
    )
    __runall__(
        c,
        "west build props-basics;props-phandles",
        [
            "rm -rf build",
            "west build --no-sysbuild --board nrf52840dk/nrf52840 -- "
            + '-DEXTRA_DTC_OVERLAY_FILE="'
            + "dts/playground/props-phandles.overlay;"
            + 'dts/playground/props-basics.overlay"',
        ],
    )
    c.run("rm -rf build")
