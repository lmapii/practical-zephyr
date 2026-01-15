"""
Invoke tasks for the 02_devicetree_semantics demo.
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
            "west build --no-sysbuild --board nrf52840dk/nrf52840 -- "
            + '-DDTC_OVERLAY_FILE="'
            + "dts/playground/props-basics.overlay;"
            + 'dts/playground/props-phandles.overlay"',
        ],
    )
    c.run("rm -rf build")
