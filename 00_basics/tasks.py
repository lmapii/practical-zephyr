"""
Invoke tasks for the 00_basics demo.
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
        "plain CMake demo",
        [
            "rm -rf build",
            "cmake -B build -DBOARD=nrf52dk/nrf52832",
            "cmake --build build -- -j4",
        ],
    )
    __runall__(
        c,
        "west demo",
        [
            "rm -rf build",
            "west build --no-sysbuild --board nrf52840dk/nrf52840",
        ],
    )
    __runall__(
        c,
        "west build.board demo",
        [
            "west config -l",
            "west config build.board nrf52840dk/nrf52840",
            "rm -rf build",
            "west build --no-sysbuild ",
            "west build --no-sysbuild --pristine",
            "west config -d build.board",
        ],
    )
    c.run("rm -rf build")
