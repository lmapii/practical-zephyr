import logging
from invoke import task


def __runall__(c, msg, cmds):
    print("###")
    print("###")
    print("###")
    print(f"### {msg}")
    print("###")
    [c.run(cmd) for cmd in cmds]  # pylint: disable=expression-not-assigned


@task
def clean(c):
    c.run("rm -rf build")


@task
def build(c, board="nrf52840dk_nrf52840"):
    c.run(f"west build --board {board}")


@task
def ci(c):
    __runall__(
        c,
        "Plain CMake demo",
        [
            "rm -rf build",
            "cmake -B build -DBOARD=nrf52dk_nrf52832",
            "cmake --build build -- -j4",
        ],
    )

    __runall__(
        c,
        "west demo",
        [
            "rm -rf build",
            "west build --board nrf52840dk_nrf52840",
        ],
    )

    __runall__(
        c,
        "west config demo",
        [
            "west config -l",
            "west config build.board nrf52840dk_nrf52840",
            "rm -rf build",
            "west build",
            "west build --pristine",
            "west config -d build.board",
        ],
    )

    c.run("rm -rf build")
