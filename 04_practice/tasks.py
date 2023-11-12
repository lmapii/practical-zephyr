"""
Invoke tasks for the 04_practice demo.
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
        ["rm -rf build", "west build --board nrf52840dk_nrf52840"],
    )
    c.run("rm -rf build")
