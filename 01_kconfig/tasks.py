"""
Invoke tasks for the 01_kconfig demo.
This file exists mainly for simplyfing the CI configuration.
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
            "west build --board nrf52840dk_nrf52840",
        ],
    )
    __runall__(
        c,
        "west release build",
        [
            "rm -rf build",
            "west build --board nrf52840dk_nrf52840 -- "
            + "-DCONF_FILE=prj_release.conf",
        ],
    )
    __runall__(
        c,
        "west extra build",
        [
            "rm -rf build",
            "west build --board nrf52840dk_nrf52840 -- "
            + '-DEXTRA_CONF_FILE="extra0.conf;extra1.conf"',
        ],
    )
    __runall__(
        c,
        "west extra release build",
        [
            "rm -rf build",
            "west build --board nrf52840dk_nrf52840 -- "
            + '-DCONF_FILE="prj_release.conf" '
            + '-DEXTRA_CONF_FILE="extra1.conf;extra0.conf"',
        ],
    )
    __runall__(
        c,
        "west hardenconfig",
        [
            "rm -rf build",
            "west build --board nrf52840dk_nrf52840 --pristine -t hardenconfig",
            "west build --board nrf52840dk_nrf52840 --pristine -t hardenconfig -- "
            + "-DCONF_FILE=prj_release.conf",
        ],
    )
    c.run("rm -rf build")
