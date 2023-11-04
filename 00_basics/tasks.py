# pylint: disable=expression-not-assigned
import logging
from invoke import task


@task
def clean(c):
    c.run("rm -rf build")


@task
def build(c, board="nrf52840dk_nrf52840"):
    c.run(f"west build --board {board}")


@task
def ci(c):
    logging.info("Plain CMake demo")
    cmds = [
        "rm -rf build",
        "cmake -B build -DBOARD=nrf52dk_nrf52832",
        "cmake --build build -- -j4",
    ]
    [c.run(cmd) for cmd in cmds]

    logging.info("west demo")
    cmds = [
        "rm -rf build",
        "west build --board nrf52840dk_nrf52840",
    ]
    [c.run(cmd) for cmd in cmds]

    logging.info("west config demo")
    cmds = [
        "west config -l",
        "west config build.board nrf52840dk_nrf52840",
        "rm -rf build",
        "west build",
        "west build --pristine",
        "west config -d build.board",
    ]
    [c.run(cmd) for cmd in cmds]

    c.run("rm -rf build")
