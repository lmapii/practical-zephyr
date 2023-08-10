
https://docs.zephyrproject.org/latest/build/zephyr_cmake_package.html#zephyr-cmake-package-search-order
https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1

- [Prerequisites](#prerequisites)
- [Setup using the nRF Connect SDK](#setup-using-the-nrf-connect-sdk)
- [Loading the development environment](#loading-the-development-environment)
  - [Analyzing the `env.sh` script provided by Nordic](#analyzing-the-envsh-script-provided-by-nordic)
  - [Creating a setup script](#creating-a-setup-script)
- [Creating an empty application skeleton](#creating-an-empty-application-skeleton)
  - [Zephyr application types](#zephyr-application-types)
  - [An freestanding application skeleton](#an-freestanding-application-skeleton)
    - [West vs. CMake](#west-vs-cmake)
    - [Kconfig](#kconfig)
    - [Sources](#sources)
- [Writing and building a dummy application](#writing-and-building-a-dummy-application)
  - [`main.c`](#mainc)
  - [`CMakeLists.txt`](#cmakeliststxt)
  - [Building the application](#building-the-application)
  - [Building with West](#building-with-west)
- [A quick glance at IDE Integrations](#a-quick-glance-at-ide-integrations)
- [Flashing and debugging](#flashing-and-debugging)
- [Summary](#summary)

## Prerequisites

- Git, also installed.
- nRF DK or similar
- why ? the zephyr documentation is great and with lots of examples available, also easy to follow.
- but .. it is easy to get lost in the dephts of the documentation. This guide walks you through the steps and pulls you back for when you might want to stop exploring.

TODO: Goal: Learn the tools step by step, especially things like `west` which are a bit much
TODO: goal of this example


## Setup using the nRF Connect SDK

The [Zephyr Getting Started Guide][zephyr-getting-started] provides details in case you want to install Zephyr on your machine. In this guide, we're starting with development kits from [Nordic][nordicsemi]. Nordic uses its own [nRF Connect SDK][nrf-connect-sdk]. This SDK is based on Zephyr and therefore its own version of it, but instead of having to go through all installation steps of the [Zephyr Getting Started Guide][zephyr-getting-started], it comes with a very handy [toolchain manager][nrf-connect-mgr] to manage different versions of the SDK and thus also Zephyr.

The nRF Connect SDK also comes with a [VS Code extension pack][nrf-connect-vscode]: We'll make use of several of the extensions but won't use the fully integrated solution since we want to understand what's going on. Go ahead and:

- Follow the [instructions][nrf-connect-mgr] to install the current toolchain,
- install the [nRF Kconfig extension for VS Code][nrf-vscode-kconfig],
- install the [nRF DeviceTree extension for VS Code][nrf-vscode-kconfig].

> **Notice:** This tutorial focuses on Linux or macOS installations and can only be used in Windows if you have the `bash` installed. We won't jump through the hoops of creating Windows compatible batch scripts or other atrocities.



## Loading the development environment

After installing the toolchain manager you can install different versions of the nRF Connect SDK. E.g., my current list of installations looks as follows:

![Toolchain Manager Screenshot](../assets/nrf-toolchain-mgr.png?raw=true)

However, when trying to use any of Zephyr's tools in your normal command line, you should notice that the commands cannot be located yet. E.g., this is the output of my `zsh` terminal when trying to execute [Zephyr's Meta Tool West][zephyr-west] (we'll go a bit further into detail about that tool later):

```zsh
$ west --version
zsh: command not found: west
```

As you can see in the above screenshot of the [toolchain manager][nrf-connect-mgr], Nordic gives you several options to launch your development environment with all paths set up correctly:

- Open a terminal,
- Generate your own `env.sh` which you can then `source` in your terminal of choice,
- Open a dedicated VS Code instance set up for the chosen SDK version.

Since we want to have a better idea of what's going on underneath, we'll use *none* of the above options. Instead, we create our own `setup.sh` script which we base on the `env.sh` that the toolchain manager generates for you.

### Analyzing the `env.sh` script provided by Nordic

First, go ahead and generate an `env.sh` script using the toolchain manager. At the time of writing, the script configures the following variables when `source`d:

- `PATH` is extended by several `bin` folders containing executables used by the nRF Connect SDK (and Zephyr).
- `GIT_EXEC_PATH`, `GIT_TEMPLATE_DIR` are set to the versions used by the SDK. These environment variables are used by Git for [sub-commands][git-env-sub] and the default repository setup when using [templates][git-env-tpl], e.g., for the initial content of your `.gitignore`.
- `ZEPHYR_TOOLCHAIN_VARIANT`, `ZEPHYR_SDK_INSTALL_DIR`, and `ZEPHYR_BASE` are Zephyr specific environment variables which we'll explain just now.

### Creating a setup script

Instead of using the provided `env.sh` script, we'll use a stripped down version of that script to explore the ecosystem and also to get to know the most important environment variables. You can find the complete [setup script](../setup.sh) in the root folder of this repository.

After installing a toolchain with the [toolchain manager][nrf-connect-mgr] you should see a tree similar to the following in your installation directory, in my case `/opt/nordic/ncs`:

```bash
tree /opt/nordic/ncs --dirsfirst -L 2 -I downloads -I tmp
/opt/nordic/ncs
├── toolchains
│   ├── 4ef6631da0
│   └── toolchains.json
└── v2.4.0
    ├── bootloader
    ├── modules
    ├── nrf
    ├── nrfxlib
    ├── test
    ├── tools
    └── zephyr
```

The toolchain and therefore required binaries are installed in a separate folder _toolchains_, whereas the actual SDK is placed in a folder with its version name. We'll make use of this to first set up our `$PATH` variable with a minimal set of `bin` directories from the installation:

```sh
ncs_install_dir=/opt/nordic/ncs
ncs_sdk_version=v2.4.0
ncs_bin_version=4ef6631da0
shell_name=$(basename "$SHELL")

paths=(
    $ncs_install_dir/toolchains/$ncs_bin_version/bin
    $ncs_install_dir/toolchains/$ncs_bin_version/opt/nanopb/generator-bin
)

for entry in ${paths[@]}; do
    export PATH=$entry:$PATH
done
```

After sourcing this script, you should now have, e.g., [Zephyr's Meta Tool West][zephyr-west] and [CMake][cmake] in your path:

```bash
$ source setup.sh
$ which cmake
/opt/nordic/ncs/toolchains/4ef6631da0/bin/cmake
$ which west
/opt/nordic/ncs/toolchains/4ef6631da0/bin/west
```

Only [two environment variables are significant][zephyr-env-important] when configuring the toolchain:
- [`ZEPHYR_SDK_INSTALL_DIR`][zephyr-env-sdk] must be set to the installation directory of the SDK,
- [`ZEPHYR_TOOLCHAIN_VARIANT`][zephyr-env-variant] selects the toolchain to use, e.g., `llvm` or in our case `zephyr`.

Once those environment variables are set, we can simply source the `zephyr-env.sh` that is provided by Zephyr itself, as shown below. This [Zephyr environment script][zephyr-env-scripts] is the recommended way to load an environment in case Zephyr is not installed globally.

```sh
# continuing setup.sh from the previous snippet
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=$ncs_install_dir/toolchains/$ncs_bin_version/opt/zephyr-sdk
source $ncs_install_dir/$ncs_sdk_version/zephyr/zephyr-env.sh
west zephyr-export
```

Instead of setting `ZEPHYR_BASE` manually as, e.g., done by the `env.sh` script generated via the Nordic toolchain manager, `ZEPHYR_BASE` is now set by the `zephyr-env.sh` script. In addition, this would also allow [using `.zephyrrc` files][zephyr-env-rc] in your application root directory.

The final command, `west zephyr-export`, is a ["Zephyr extension command"][zephyr-west-export] that installs the required _CMake_ packages that we'll need in the next steps when using `find_package` to locate Zephyr in the `CMakeLists.txt` of the project.



## Creating an empty application skeleton

Before creating our first files, we need to have a quick look at the application types that are supported by Zephyr.

### Zephyr application types

Zephyr supports different [application types][zephyr-app-types], essentially depending on the location of your application in relation to Zephyr:

- [Freestanding applications][zephyr-app-freestanding] exist independently, meaning Zephyr is not part of the application's repository or filetree. The relation with Zephyr is only created in the build process when using `find_package` to locate Zephyr, typically based on the `ZEPHYR_BASE` environment variable that we've set in the above steps.
- [Workspace applications][zephyr-app-workspace] use a [west workspace][zephyr-west-workspace]: For such applications *west* is used to initialize your workspace, e.g., based on a `west.yml` manifest file. The manifest file is used by *west* to create complex workspaces: For each external dependency that is used in your project, the location and revision is specified in the manifest file. West then uses this manifest to populate your workspace. Kind of like a very, very powerful version of [Git submodules][git-submodules].
- [Zephyr repository applications][zephyr-app-repository] live within the Zephyr repository itself, e.g., demo applications that are maintained with Zephyr. You could add applications in a fork of the Zephyr repository, but this is not very common.

The most common and easiest approach is to start with a freestanding application: You'll only need a couple of files to get started. As with the nRF Connect SDK, Zephyr is located in a known installation directory that is configured via the environment.

In more advanced projects you typically want to have all dependencies in your workspace and will therefore switch to a workspace application type: External dependencies will be cloned directly into your project by *west* to avoid dependencies to local installations.

For this guide, we'll rely entirely on the [freestanding application type][zephyr-app-freestanding].

### An freestanding application skeleton

The steps to create an application from scratch is also documented in the [Zephyr documentation][zephyr-app-create]. We'll simply repeat them here and add some explanations, where necessary. Start by creating the following skeleton:

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

Something that you should notice immediately, is that we're using a [CMakeLists.txt](CMakeLists.txt). This is due to the fact that Zephyr's build system is based on [CMake][cmake]. You've read that right: While Zephyr provides its own meta tool `west`, underneath there is yet another meta build system `cmake`.

#### West vs. CMake

[West][zephyr-west] is Zephyr's "swiss army knife tool": It is mainly used for managing multiple repositories but also provides [Zephyr extension commands][zephyr-west-cmd-ext] for building, flashing and debugging applications. [CMake][cmake] on the other hand is used exclusively for building your application.

Since there exist some oddities when using `west` for building your application (we'll see that in a minute), and since we want to learn stuff, for now we'll just focus on how things work by sticking to `cmake` and switch back to `west` after exploring the build process.

#### Kconfig

In our application skeleton there is also a very suspicious [prj.conf](prj.conf) file. This file must exist for any application, even if empty, and is used by the [Kconfig configuration system][zephyr-kconfig]. We'll go into detail about [Kconfig][zephyr-kconfig] (and *DeviceTree*) later in this guide.

For now simply keep in mind that we'll be using this configuration file to tell the build system which modules and which features we want to use in our application. Anything that we don't plan on using will be deactivated, decreasing the size of our application and reducing compilation times.

#### Sources

Aside from our build an configuration file, we've created a (still empty) [src/main.c](./src/main.c) file. The convention is to place all sources in this `src` root folder, but as your application grows you might need a more complex setup. For now that'll do.


## Writing and building a dummy application

### `main.c`

Our `main.c` file contains our application code. For now we'll just provide a dummy application that does nothing but send our MCU back to sleep repeatedly.

```c
#include <zephyr/kernel.h>

void main(void)
{
    while (1)
    {
        k_msleep(100U); // Sleep for 100 ms.
    }
}
```

Notice that this is a plain infinite loop and it doesn't make any use of threads yet - something that we'll definitely change later. For now just keep in mind that [applications _can_ operate without threads][zephyr-nothread], though this serverely limits the functionality provided by Zephyr.

### `CMakeLists.txt`

TODO: set(BOARD ...) not to be used

Now we come to the heart of this first step towards Zephyr: The build configuration. In case you're not familiar at all with *CMake*, there exist several articles and even books about ["Effective Modern CMake"][cmake-gist]: *CMake* has evolved significantly, and only certain patterns should be used for new applications.

Let's go through the steps to create our [CMakeLists.txt](CMakeLists.txt) for this empty application:

```cmake
cmake_minimum_required(VERSION 3.20.0)

set(BOARD nrf52840dk_nrf52840)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
```

TODO: real required minimal version
`zephyr/cmake/modules/zephyr_default.cmake`

- `cmake_minimum_required` just indicates the allowed CMake versions.
- The `set` function writes the board that we're building our application for to the variable `BOARD`. This step is optional and can actually be specified during the build step as parameter.
- Then we go ahead and load `Zephyr` using the `find_package` function. In our [setup script][#creating-a-setup-script] we've exported the `ZEPHYR_BASE` environment variable which is now passed as a hint to the `find_package` function to locate the correct Zephyr installation.

> **Note:** There are several ways to use the Zephyr CMake package, each of which is described in detail in the [Zephyr CMake Package documentation][zephyr-cmake-pkg]. Have a look at the [Zephyr CMake package source code][zephyr-cmake-pkg-source] for details.

As mentioned, setting the `BOARD` variable is optional and usually only done if you want to build for only one board. The value for the `BOARD` can be chosen in different ways, e.g., using an environment variable. The Zephyr build system determines the final value for `BOARD` in a pre-defined order; refer to the [documentation for the application CMakeLists.txt][zephyr-cmakelists-app] for details.

> **Note**: TODO: update. Personally, while experimenting I typically specify the `BOARD` in the `CMakeLists.txt` file since I don't really remember the exact board names. You can always dump the list of supported boards using the command `west boards`.

Now that Zephyr is available, we can go ahead and add the application:

```cmake
project(
    EmptyApp
    VERSION 0.1
    DESCRIPTION "Empty zephyr application."
    LANGUAGES C
)

target_sources(
    app
    PRIVATE
    src/main.c
)
```

- The `project` function defines the name and settings of the application project.
- Then, we simply need to add our sources to the `app` target.

But wait. What's this `app` target and why don't we need to specify an executable? This is all done within the Zephyr CMake package: The Zephyr package already defines your executable and also provides this `app` library target which is intended to be used for all application code. At the time of writing, this `app` target is constructed by the following CMake files in the Zephyr project:

`zephyr/cmake/modules/kernel.cmake`
```cmake
zephyr_library_named(app)
```

`zephyr/cmake/modules/extensions.cmake`
```cmake
macro(zephyr_library_named name)
  set(ZEPHYR_CURRENT_LIBRARY ${name})
  add_library(${name} STATIC "")
  zephyr_append_cmake_library(${name})
  target_link_libraries(${name} PUBLIC zephyr_interface)
endmacro()
```

### Building the application

With the [CMakeLists.txt](CMakeLists.txt) in place we can go ahead and build the application. There are several ways to do this and if you're familiar with CMake the below commands are no surprise. What we want to show, however, is the slight differences between using CMake or West when building your application. Lets first use CMake:

```bash
$ cmake -B ../build
$ cmake --build ../build -- -j4
```

With this, the `../build` folder should contain the application built for the `BOARD` specified in the [CMakeLists.txt](CMakeLists.txt) file. The following command allows us to pass the `BOARD` to CMake directly and thereby either **override** a given `set(BOARD ...)` instruction or to provide the `BOARD` in case it hasn't been specified at all:

```bash
# Build for a different board
$ cmake -B ../build -DBOARD=nrf52dk_nrf52832
```

Let's try to build using West:

```bash
$ rm -rf ../build
$ west build -d ../build
WARNING: This looks like a fresh build and BOARD is unknown; so it probably won't work. To fix, use --board=<your-board>.
Note: to silence the above message, run 'west config build.board_warn false'
<truncated output>
```

> **Note:** The default build directory of West is a `build` folder within the same directory. If run without `-d ../build`, West will create a `build` folder in the current directory for the build.

The build succeeds, but there's a warning indicating that no board has been specified. How is that possible? West itself typically still expects a "board configuration" or the `--board` parameter. The reason for this is simple: West is much more than just a wrapper for CMake and therefore doesn't parse the provided `CMakeLists.txt` file. It just warns you that you may have forgotten to specify the `board` but still proceeds to call CMake.

Before looking yet another step deeper into West, comment or delete the `set(BOARD ...)` instruction in your `CMakeLists.txt` file.

### Building with West

As mentioned, West needs an indication which board is being used by your project. Just like plain CMake, this could be done by specifying the `BOARD` as environment variable, or by passing the board to `west build`:

```bash
$ west build --board nrf52840dk_nrf52840 -d ../build
# or using an environment variable
$ export BOARD=nrf52840dk_nrf52840
$ west build -d ../build
```

These builds run without warning and without the `set(BOARD ...)` instruction in the `CMakeLists.txt` file. Once the build has been created it is no longer necessary to pass the board to `west`.

Alternatively, we can use [West's own build configuration options][zephyr-west-build-cfg] to fix the board in the executing terminal. These options are similar to environment variables but are configuration options only relevant for West. We can list the current West configuration options using `west config -l`:

```bash
$ west config -l
manifest.path=nrf
manifest.file=west.yml
zephyr.base=zephyr
```

The listed configuration options are part of [West's built-in configuration options][zephyr-west-builtin-cfg] and come from the West workspace configuration. Since we're using a freestanding application we'll ignore these options for now and focus on our build settings. We can add a default board configuration to our settings as follows:

```bash
$ west config build.board nrf52840dk_nrf52840
$ west config -l
manifest.path=nrf
manifest.file=west.yml
zephyr.base=zephyr
build.board=nrf52840dk_nrf52840
# It is also possible to delete the option again
# using `west config -d build.board`
```

> **Note:** The west configuration options can also be used to define arguments for CMake. This can be useful when debugging or when defining your own CMake caches, refer to the [documentation][zephyr-west-cmake-cfg] for details.

Now we can run `west build` in our current terminal without specifying a board.

```bash
$ west build -d ../build
```

One last parameter for `west build` that is worth mentioning is `--pristine`: Instead deleting the `build` folder, you can also use the `--pristine` to generate a new build system:

```bash
$ west build -d ../build --pristine
```


## A quick glance at IDE Integrations

TODO: here

as mentioned, IDEs are intentionally ignored since this is up to you
but here are a few hints
also notice that before we used -d ../build, because vscodes cmake extension picks up build folder in the root directory
code completion is important

`zephyr/cmake/modules/kernel.cmake`
```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE CACHE BOOL
    "Export CMake compile commands. Used by gen_app_partitions.py script"
    FORCE
)
```

## Flashing and debugging



## Summary

What did we learn about the environment
Which west commands did we see

Next up: KConfig and DeviceTree.

TODO: at the beginning we'll use west only for building, debugging and flashing
https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html

workspaces will come up later (this is actually what can be a bit confusing in the docs, since wests actual purpose is something that you'll only make use of much later in your projects).

TODO: using a different devkit? `west boards` or is there a list in the docs?



[zephyr-getting-started]: https://docs.zephyrproject.org/latest/develop/getting_started/index.html
[nordicsemi]: https://www.nordicsemi.com/
[nrf-connect-sdk]: https://www.nordicsemi.com/Products/Development-software/nrf-connect-sdk
[nrf-connect-mgr]: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/assistant.html#install-toolchain-manager
[nrf-connect-vscode]: https://nrfconnect.github.io/vscode-nrf-connect/
[nrf-vscode-kconfig]: https://marketplace.visualstudio.com/items?itemName=nordic-semiconductor.nrf-kconfig
[nrf-vscode-devicetree]: https://marketplace.visualstudio.com/items?itemName=nordic-semiconductor.nrf-devicetree
[zephyr-west]: https://docs.zephyrproject.org/latest/develop/west/index.html
[git-env-sub]: https://git-scm.com/book/en/v2/Git-Internals-Environment-Variables
[git-env-tpl]: https://git-scm.com/docs/git-init#_template_directory
[cmake]: https://cmake.org/
[cmake-gist]: https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1
[zephyr-env-important]: https://docs.zephyrproject.org/latest/develop/env_vars.html#important-environment-variables
[zephyr-env-variant]: https://docs.zephyrproject.org/latest/develop/beyond-GSG.html#install-a-toolchain
[zephyr-env-sdk]: https://docs.zephyrproject.org/latest/develop/env_vars.html#envvar-ZEPHYR_SDK_INSTALL_DIR
[zephyr-env-scripts]: https://docs.zephyrproject.org/latest/develop/env_vars.html#zephyr-environment-scripts
[zephyr-env-rc]: https://docs.zephyrproject.org/latest/develop/env_vars.html#option-3-using-zephyrrc-files
[zephyr-west-export]: https://docs.zephyrproject.org/latest/develop/west/zephyr-cmds.html#installing-cmake-packages-west-zephyr-export
[zephyr-app-types]: https://docs.zephyrproject.org/latest/develop/application/index.html#application-types
[zephyr-app-freestanding]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-freestanding-app
[zephyr-app-workspace]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-workspace-app
[zephyr-app-repository]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-repo-app
[zephyr-west-workspace]: https://docs.zephyrproject.org/latest/develop/west/workspaces.html#west-workspaces
[zephyr-app-create]: https://docs.zephyrproject.org/latest/develop/application/index.html#creating-an-application-by-hand
[git-submodules]: https://git-scm.com/book/en/v2/Git-Tools-Submodules
[zephyr-kconfig]: https://docs.zephyrproject.org/latest/build/kconfig/index.html
[zephyr-west-cmd-ext]: https://docs.zephyrproject.org/latest/develop/west/zephyr-cmds.html
[zephyr-west-build-cfg]: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html#configuration-options
[zephyr-nothread]: https://docs.zephyrproject.org/latest/kernel/services/threads/nothread.html#nothread
[zephyr-cmake-pkg]: https://docs.zephyrproject.org/latest/build/zephyr_cmake_package.html
[zephyr-cmake-pkg-source]: https://docs.zephyrproject.org/latest/build/zephyr_cmake_package.html#zephyr-cmake-package-source-code
[zephyr-cmakelists-app]: https://docs.zephyrproject.org/latest/develop/application/index.html#application-cmakelists-txt
[zephyr-west-builtin-cfg]: https://docs.zephyrproject.org/latest/develop/west/config.html#built-in-configuration-options
[zephyr-west-cmake-cfg]: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html#permanent-cmake-arguments

<!-- ## Setup

Very verbose log:
```bash
cmake --build build -- -vvv -DEXTRA_FLAGS="-save-temps" > build.log 2>&1
```

Building in parallel with `cmake`:
```bash
cmake --build build -- -j 10
```

Others
```bash
# Only clean the application/objects, but not generated files.
cmake --build build -- clean
west build -t clean
# Delete _all_ files in "build", including generated files.
cmake --build build -- pristine
west build -t pristine
# Notice that after a pristine clean, for `west build` the board must be specified again.
``` -->
