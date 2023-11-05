

- [Outline](#outline)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
  - [Setup using Nordic's toolchain manager](#setup-using-nordics-toolchain-manager)
  - [Loading the development environment](#loading-the-development-environment)
  - [Analyzing the `env.sh` script provided by Nordic](#analyzing-the-envsh-script-provided-by-nordic)
  - [Creating a setup script](#creating-a-setup-script)
- [Creating an empty application skeleton](#creating-an-empty-application-skeleton)
  - [Zephyr application types](#zephyr-application-types)
  - [A skeleton freestanding application](#a-skeleton-freestanding-application)
    - [West vs. CMake](#west-vs-cmake)
    - [Kconfig](#kconfig)
    - [Sources](#sources)
- [Writing and building a dummy application](#writing-and-building-a-dummy-application)
  - [`main.c`](#mainc)
  - [`CMakeLists.txt`](#cmakeliststxt)
  - [Building the application](#building-the-application)
    - [Building with CMake](#building-with-cmake)
    - [Building with West](#building-with-west)
- [A quick glance at IDE integrations](#a-quick-glance-at-ide-integrations)
- [Flashing and debugging](#flashing-and-debugging)
  - [Debugging ARM Cortex-M targets in `vscode`](#debugging-arm-cortex-m-targets-in-vscode)
- [Conclusion](#conclusion)
- [Further reading](#further-reading)



## Yet another Zephyr series <!-- omit in toc -->

[Zephyr's own documentation][zephyr-getting-started] aside, several very good article series exist already, e.g., the _"Getting Started with Zephyr"_ series by [Mohammed Billoo][billoo], or the Zephyr notes by [Abhijit Boseji][boseji]. Nordic Semiconductor also launched its [DevAcademy][nordicsemi-dev-academy], which contains a great course on the [nRF Connect SDK Fundamentals][nrf-connect-sdk-fundamentals]: Since the _nRF Connect SDK_ is based on Zephyr, this course also teaches about Zephyr.

So why another article series, and what's different? When I first started looking into Zephyr the learning curve felt quite overwhelming. Even as an embedded developer with a decent amount of experience, understanding the key concepts and tools like `Kconfig` or `devicetree` for creating even a simple application, had me chasing down the rabbit hole, following link after link after link, night after night - and it was exhausting.

What I'm trying to achieve with this article series is the following: If you're working a full-time job and would still like to get started with Zephyr, but don't have the energy to set up your own environment, to dive deeper into the docs, this article series will guide you through __Zephyr's fundamental concepts__. Of course, you'll learn most if you follow along with programming, but all code, including snippets of generated code and build logs, are included in the articles of this series, so even just reading should give you a good idea about how Zephyr works.

After this series, you should understand all of Zephyr's fundamental concepts. The learning curve flattens, and you'll be able to really get started without getting lost in Zephyr's documentation.

> **Note:** This series is **not** for you if you're not interested in details and how things work, but rather just want to _use_ Zephyr. We'll be going through _generated code and configuration files_! Also, if you're experienced with Linux and have already seen how _Kconfig_ and _devicetree_ work, you might find the articles of this series too detailed.



## Outline

This series covers "Zephyr's fundamental concepts". What does that mean? In short "Zephyr's fundamental concepts" are essentially its [build and configuration systems][zephyr-build-and-cfg-systems], and that's what we'll explore in detail:

> _Basics_

First, we walk through the very basics of installing Zephyr and creating a minimal, dummy application. We'll see how we can compile, flash, and debug an application, and how Zephyr's `west` tool fits into all of this.

> _Kconfig_

In our dummy application, we'll already stumble upon the configuration tool [Kconfig][zephyr-kconfig]. We'll see how we can explore, modify, and persist so-called _Kconfig symbols_, and will create our own application-specific symbol. Since _Kconfig_ is also used to configure build settings, we'll see how to use _Kconfig_ for _build types_.

> _Devicetree basics_

Zephyr uses a tree data structure called _devicetree_ to describe hardware: All supported microcontrollers and boards are represented by a set of _devicetree sources_, which use their own syntax. First, we'll skim through some devicetree files in Zephyr, and then create our own files to explore the _syntax_.

> _Devicetree bindings_

After understanding the _devicetree_ syntax and basics, we'll now use so-called _bindings_ to add _semantics_ and therefore _meaning_ to our devicetree. Only via devicetree _bindings_, Zephyr knows how to translate the provided devicetree into source code. Creating bindings for our own nodes, we'll look at the generated output, and finally how we can access it via the Zephyr devicetree API.

> _Workspaces_

Throughout this article series, for the sake of simplicity, we'll use so-called _freestanding_ applications: Such applications require Zephyr to be installed elsewhere and do not include its sources. For professional applications, Zephyr's `west` tool supports so-called _workspaces_: In workspaces, _all_ sources, their location, and their versions are specified in a _manifest_ file. `west` uses this manifest to populate the workspace, eliminating references to files outside of the workspace. We'll see how we can create a _minimal_ workspace application using `west`.

This is where the journey **ends**. After this series, you should find it easy - or at least easier - to explore, understand, and apply advanced concepts such as:

- [application version management](https://docs.zephyrproject.org/latest/build/version/index.html)
- using the actual [Zephyr operating system and its communication primitives](https://docs.zephyrproject.org/latest/services/index.html),
- creating custom [_device drivers_](https://docs.zephyrproject.org/latest/kernel/drivers/index.html) or integrating a [_custom board_](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html),
- using integrated bootloaders such as [_Mcuboot_](https://docs.zephyrproject.org/latest/services/device_mgmt/dfu.html#mcuboot),
- [testing Zephyr](https://docs.zephyrproject.org/latest/develop/test/index.html) applications, e.g., using its [_Twister_](https://docs.zephyrproject.org/latest/develop/test/twister.html) test runner,
- running [static code analysis](https://docs.zephyrproject.org/latest/develop/sca/index.html) with the integrated [_code checker_](https://docs.zephyrproject.org/latest/develop/sca/codechecker.html),
- and many more ...

... but we won't cover any of the above. All example code used throughout the articles is available in the [_practical-zephyr_ GitHub repository][practical-zephyr].



## Prerequisites

In general, you should be familiar with embedded software since we won't cover the basics. Some knowledge in using build systems such _CMake_ doesn't hurt, but is not really necessary. In case you don't know at all what Zephyr is, have a short look at the [introduction in the official documentation][zephyr-introduction].

The practical examples in this series use [Nordic's][nordicsemi] [nRF52840 development kit][nordicsemi-nrf52840-dk]. In case you don't have such a board, don't worry. You should be able to follow along using most of the boards in [Zephyr's long list of supported boards][zephyr-boards] - or using an emulation target, though we won't cover that. Most of the content of this series does not require any hardware at all, we'll just _compile_ the project for a specific board for simplicity.

Whenever relevant, we'll use [Visual Studio Code][vscode] as the editor of choice. However, **using `vscode` is of course entirely optional** and not required at all. Only in certain sections, e.g., debugging, we'll show how using `vscode` and some plugins can make your life easier - and we won't cover any other editor.

We won't assume any type of operating system, but - for the sake of simplicity - we'll be exclusively using Linux shell commands. Windows users should have a "`bash`-compatible" environment - or `WSL` - installed. We won't jump through the hoops of creating Windows-compatible shell scripts, batch files, or other atrocities.



## Installation

Let's get started - with a shortcut. Installation and toolchain management is - and probably always will be - a very biased topic. This article series is not about solving that problem. Therefore, I'm deliberately choosing a shortcut: We'll download "all things Zephyr" using Nordic's [toolchain manager][nrf-connect-mgr] instead of following the complete installation procedure. This is not required, but convenient.

Instead of using Nordic's full-blown IDE solution, however, we'll create our own shell script that sets up our terminal. This demonstrates some of the environment variables used by Zephyr and allows you to use a plain terminal to follow along.

> **Note:** In case you want a "Zephyr only" installation without downloading Nordic's software, feel free to follow the installation procedure in the [official documentation][zephyr-getting-started] - and I'll meet you again in the [next section](#creating-an-empty-application-skeleton)!


### Setup using Nordic's toolchain manager

Nordic provides the [nRF Connect SDK][nrf-connect-sdk] for its microcontrollers. This SDK is based on Zephyr and therefore its own version of it (plus some vendor-specific extras), but instead of having to go through all installation steps of the [getting started guide][zephyr-getting-started], it comes with a very handy [toolchain manager][nrf-connect-mgr] to manage different versions of the SDK and thus also Zephyr.

The nRF Connect SDK also comes with a [`vscode` extension pack][nrf-connect-vscode]: We'll make use of several of the extensions, but won't use the fully integrated solution since we want to understand what's going on. Go ahead and:

- Follow the [toolchain manager's instructions][nrf-connect-mgr] to install the current toolchain,
- _[optional]_ install the [nRF Kconfig extension for `vscode`][nrf-vscode-kconfig],
- _[optional]_ install the [nRF DeviceTree extension for `vscode`][nrf-vscode-kconfig].

> **Note:** If you like Nordic's [nRF Connect for `vscode` extension pack][nrf-connect-vscode], you can now also skip the _toolchain manager_ installation step and install the toolchain straight from within `vscode`.

If you're not convinced and want to install the SDK manually, you can also have a look at the steps performed in the [Dockerfile](https://github.com/lmapii/practical-zephyr/blob/main/builder.Dockerfile) in the [accompanying GitHub repository][practical-zephyr]. Notice that this _Dockerfile_ is tailored to the needs of this article series and is _not_ a generic solution. If you're looking for something more generic, have a look at [Zephyr's docker-image repository][zephyr-gh-docker].


### Loading the development environment

After installing the toolchain manager you can install different versions of the nRF Connect SDK. E.g., my current list of installations looks as follows:

![Toolchain Manager Screenshot](../assets/nrf-toolchain-mgr.png?raw=true)

However, when trying to use any of Zephyr's tools in your normal command line, you should notice that the commands cannot be located yet. E.g., this is the output of my `zsh` terminal when trying to execute [Zephyr's meta-tool `west`][zephyr-west] (we'll see the tool later):

```zsh
$ west --version
zsh: command not found: west
```

As you can see in the above screenshot of the [toolchain manager][nrf-connect-mgr], Nordic gives you several options to launch your development environment with all paths set up correctly:

- Open a terminal,
- generate your own `env.sh` which you can then `source` in your terminal of choice,
- open a dedicated `vscode` instance set up for the chosen SDK version.

Since we want to have a better idea of what's going on underneath, we'll use *none* of the above options. Instead, we create our own `setup.sh` script which we base on the `env.sh` that the toolchain manager generates for you.


### Analyzing the `env.sh` script provided by Nordic

First, go ahead and generate an `env.sh` script using the toolchain manager. At the time of writing, the script configures the following variables when `source`d:

- `PATH` is extended by several `bin` folders containing executables used by the nRF Connect SDK (and Zephyr).
- `GIT_EXEC_PATH` and `GIT_TEMPLATE_DIR` are set to the versions used by the SDK. These environment variables are used by Git for [sub-commands][git-env-sub], and the default repository setup when using [templates][git-env-tpl], e.g., for the initial content of your `.gitignore`.
- `ZEPHYR_TOOLCHAIN_VARIANT`, `ZEPHYR_SDK_INSTALL_DIR`, and `ZEPHYR_BASE` are Zephyr-specific environment variables which we'll explain just now.


### Creating a setup script

Instead of using the provided `env.sh` script, we'll use a stripped-down version of that script to explore the ecosystem and also to get to know the most important environment variables. You can find the complete [setup script](https://github.com/lmapii/practical-zephyr/blob/main/setup.sh) in the root folder of the [accompanying GitHub repository][practical-zephyr].

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

The toolchain and therefore all required binaries are installed in a separate folder _toolchains_, whereas the actual SDK is placed in a folder with its version name. We'll make use of this to first set up our `$PATH` variable with a minimal set of `bin` directories from the installation:

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
    export PATH=$PATH:$entry
done
```

> **Note:** Depending on whether or not you want to replace _all_ binaries - including `git` - with Nordic's installation, _append_ or _prepend_ to `PATH`. In the above example, we _append_ and thus use local installations over the installation provided by the toolchain installer, if available.

After sourcing this script, you should now have, e.g., [Zephyr's meta-tool `west`][zephyr-west] and [CMake][cmake] in your path:

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

Once those environment variables are set, we can source the `zephyr-env.sh` that is provided by Zephyr itself, as shown below. This [Zephyr environment script][zephyr-env-scripts] is the recommended way to load an environment in case Zephyr is not installed globally.

```sh
# continuing setup.sh from the previous snippet
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=$ncs_install_dir/toolchains/$ncs_bin_version/opt/zephyr-sdk
source $ncs_install_dir/$ncs_sdk_version/zephyr/zephyr-env.sh
west zephyr-export
```

Instead of setting `ZEPHYR_BASE` manually as, e.g., done by the `env.sh` script generated via the Nordic toolchain manager, `ZEPHYR_BASE` is now set by the `zephyr-env.sh` script. In addition, this also allows [using `.zephyrrc` files][zephyr-env-rc] in your application root directory - a feature we won't cover in this series.

The final command, `west zephyr-export`, is a ["Zephyr extension command"][zephyr-west-export] that installs the required _CMake_ packages that we'll need in the next steps when using `find_package` to locate Zephyr in a `CMakeLists.txt` file.

That's it for the environment, we can now go ahead and create applications. One last time I'd like to emphasize that this was just a warm-up to get started with Zephyr. It is not required or recommended to use such a `setup.sh` script, it's just many of the possibilities you have when bringing up your development environment.



## Creating an empty application skeleton

Now that we have a working installation, we can start with the important parts of this article series. Before creating our first files, however, we'll review the application _types_ supported by Zephyr.

### Zephyr application types

Zephyr supports different [application types][zephyr-app-types], differentiated essentially depending on the location of your application in relation to Zephyr:

- [Freestanding applications][zephyr-app-freestanding] exist independent of Zephyr's location, meaning Zephyr is not part of the application's repository or file tree. The relation with Zephyr is only established in the build process, when using `find_package` to locate Zephyr, typically based on the `ZEPHYR_BASE` environment variable.
- [Workspace applications][zephyr-app-workspace] use a [`west` workspace][zephyr-west-workspace]: For such applications, `west` is used to initialize your workspace, e.g., based on a `west.yml` manifest file. The manifest file is used by `west` to create complex workspaces: For each external dependency that is used in your project, the location and revision are specified in the manifest file. West then uses this manifest to populate your workspace. Kind of like a very, very powerful version of [Git submodules][git-submodules].
- [Zephyr repository applications][zephyr-app-repository] live within the Zephyr repository itself, e.g., demo applications that are maintained with Zephyr. You could add applications in a fork of the Zephyr repository, but this is not very common.

The most common and easiest approach is to **start** with a freestanding application: You'll only need a couple of files to get started. As with the nRF Connect SDK, Zephyr is located in a known installation directory that is configured via the environment.

In more advanced projects you typically want to have all dependencies in your workspace and will therefore switch to a workspace application type: External dependencies will be cloned directly into your project by `west` to avoid dependencies to local installations. We'll get to that at the end of this article series.

For now, we'll rely entirely on the [freestanding application type][zephyr-app-freestanding].

### A skeleton freestanding application

The steps to create an application from scratch are also documented in [Zephyr's documentation][zephyr-app-create]. We'll simply repeat them here and add some explanations, where necessary. Start by creating the following skeleton:

```bash
$ tree --charset=utf-8 --dirsfirst
.
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```

One thing that you should notice immediately, is that we're using a [CMakeLists.txt](CMakeLists.txt). This is due to the fact that Zephyr's build system is based on [CMake][cmake]. You've read that right: While Zephyr provides its own meta-tool `west`, underneath there is yet another meta build system _CMake_ that will in turn, e.g., use `make` or `ninja`.

#### West vs. CMake

[West][zephyr-west] is Zephyr's "Swiss army knife tool": It is mainly used for managing multiple repositories but also provides [Zephyr extension commands][zephyr-west-cmd-ext] for building, flashing, and debugging applications. [CMake][cmake], on the other hand, is used exclusively for building your application.

Since we want to learn how things work underneath, for the following steps we'll be using _CMake_ first, and switch back to `west` after exploring the build process.

#### Kconfig

In our application skeleton, there is also a very suspicious [prj.conf](prj.conf) file. This file must exist for any application, even if empty, and is used by the [Kconfig configuration system][zephyr-kconfig]. We'll go into detail about [Kconfig][zephyr-kconfig] (and *devicetree*) later in this series.

For now, simply keep in mind that we'll be using this configuration file to tell the build system which modules and which features we want to use in our application. Anything that we don't plan on using will be deactivated, decreasing the size of our application and reducing compilation times.

#### Sources

Aside from our build and configuration file, we've created a still empty [src/main.c](./src/main.c) file. The convention is to place all sources in this `src` folder, but as your application grows you might need a more complex setup. For now, that'll do.



## Writing and building a dummy application

### `main.c`

Our `main.c` file contains our application code. We'll just provide a dummy application that does nothing but send our MCU back to sleep repeatedly:

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

Notice that this is a plain infinite loop and it doesn't make any use of threads yet - something that you'll definitely change in a more complex application: In the end, it makes limited sense to use Zephyr without its operating system. For now keep in mind that [applications _can_ operate without threads][zephyr-nothread], though this severely limits the functionality provided by Zephyr.

### `CMakeLists.txt`

Now we come to the heart of this first step towards Zephyr: The build configuration. In case you're not familiar at all with *CMake*, there are several articles and even books about ["Effective Modern CMake"][cmake-gist]: *CMake* has evolved significantly, and only certain patterns should be used for new applications. We won't explain *CMake* here, but if you're an experienced developer you should understand the basics just fine when reading along.

Let's go through the steps to create our [CMakeLists.txt](CMakeLists.txt) for this empty application:

```cmake
cmake_minimum_required(VERSION 3.20.0)

set(BOARD nrf52840dk_nrf52840)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
```

- `cmake_minimum_required` just indicates the allowed CMake versions. Zephyr has its own requirements and maintains its minimal required CMake version in `zephyr/cmake/modules/zephyr_default.cmake`.
- The `set` function writes the board that we're building our application for to the variable `BOARD`. This step is optional and can actually be specified during the build step as a parameter. `nrf52840dk_nrf52840` is Zephyr's name for the [nRF52840 development kit][nordicsemi-nrf52840-dk].
- Then we go ahead and load `Zephyr` using the `find_package` function. In our [setup script][#creating-a-setup-script] we've exported the `ZEPHYR_BASE` environment variable, which is now passed as a hint to the `find_package` function to locate the correct Zephyr installation.

> **Note:** There are several ways to use the Zephyr CMake package, each of which is described in detail in [Zephyr's CMake package documentation][zephyr-cmake-pkg]. You can also have a look at the [Zephyr CMake package source code][zephyr-cmake-pkg-source] for details.

As mentioned, setting the `BOARD` variable is optional and usually not done. The value for the `BOARD` can be chosen in different ways, e.g., using an environment variable, or passed as a parameter during the build. The Zephyr build system determines the final value for `BOARD` in a pre-defined order; refer to the [documentation for the application CMakeLists.txt][zephyr-cmakelists-app] for details.

> **Note**: Zephyr's goal is to support multiple boards and multiple MCUs. Once we switch to using `west` for building, it makes sense to pass the board as a parameter and _not_ specify it in `CMakeLists.txt`. In case you're having trouble remembering the exact board name, you can always dump the list of supported boards using the command `west boards`.

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

But wait. What's this `app` target and why don't we need to specify an executable? This is all done within the Zephyr CMake package: The package already defines your executable and also provides this `app` library target which is intended to be used for all application code. At the time of writing, this `app` target is constructed by the following CMake files in the Zephyr project:

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

With the [CMakeLists.txt](CMakeLists.txt) in place we can go ahead and build the application. There are several ways to do this and if you're familiar with CMake, the below commands are no surprise. What we want to show, however, is the slight differences between using CMake or `west` when building your application. Let's first use CMake:

#### Building with CMake

```bash
$ cmake -B ../build
$ cmake --build ../build -- -j4
```

> **Note:** The [GitHub repository][practical-zephyr] uses subfolders for all example applications. Using a build directory `../build` located in the root of the repository allows an editor such as `vscode` to pick up the compile commands independent of the example that is being built. In case you're using `vscode`, have a look at the provided [`settings.json`](https://github.com/lmapii/practical-zephyr/blob/main/.vscode/settings.json) file.

With this, the `../build` folder contains the application built for the `BOARD` specified in the [CMakeLists.txt](CMakeLists.txt) file. The following command allows passing the `BOARD` to CMake directly and thereby either **overrides** a given `set(BOARD ...)` instruction or provides the `BOARD` in case it hasn't been specified at all:

```bash
# Build for a specific board determined during build time.
$ cmake -B ../build -DBOARD=nrf52dk_nrf52832
```

Let's try to build the application using `west`:

```bash
$ rm -rf ../build
$ west build -d ../build
WARNING: This looks like a fresh build and BOARD is unknown; so it probably won't work. To fix, use --board=<your-board>.
Note: to silence the above message, run 'west config build.board_warn false'
<truncated output>
```

> **Note:** The default build directory of `west` is a `build` folder within the same directory. If run without `-d ../build`, `west` will create a `build` folder in the current directory for the build.

The build seems to succeed, but there's a warning indicating that no board has been specified. How is that possible? `west` itself typically still expects a "board configuration" or the `--board` parameter. The reason for this is simple: `west` is much more than just a wrapper for CMake and therefore doesn't parse the provided `CMakeLists.txt` file - where we specified the `BOARD`. It just warns you that you may have forgotten to specify the `board` but still proceeds to call CMake.

Before looking yet another step deeper into West, comment or **delete** the `set(BOARD ...)` instruction in your `CMakeLists.txt` file.


#### Building with West

As mentioned, `west` needs an indication of which board is being used by your project. Just like plain CMake, this could be done by specifying the `BOARD` as an environment variable, or by passing the board to `west build`:

```bash
# pass the board as argument
$ west build --board nrf52840dk_nrf52840 -d ../build
# or using an environment variable
$ export BOARD=nrf52840dk_nrf52840
$ west build -d ../build
```

Underneath, `west` sets up the build directory and then simply invokes CMake. Thus, everything build-related is still plain old CMake. The above builds run without warning.

> **Note:** The `build` command is actually a so-called [`west` extension command][zephyr-west-cmd-ext] and is not built into `west` by default. You could thus, e.g., use `west` without Zephyr as a repository manager in a different context. Zephyr provides extension commands such as [`build`, `flash`, and `debug`][zephyr-west-cmd-ext-zephyr] in its own `west` extension such that applications can be built, flashed, and debugged in the same way, independent of, e.g., the debugger or programmer that is used. If you're interested in how Zephyr's extensions work, have a look at `zephyr/scripts/west-commands.yml` and the corresponding `west` extension scripts.

Once the build folder has been created using any of the above commands, it is no longer necessary to pass the board to `west`:

```bash
# in case the build directory exists already ...
$ if test -d ../build; then echo "build directory exists!"; fi
build directory exists!
# ... it is no longer necessary to specify the board:
$ west build -d ../build
```

Alternatively, we can use [West's own build configuration options][zephyr-west-build-cfg] to fix the board in the executing terminal. These options are similar to environment variables but are configuration options _only_ relevant to West. We can list the current West configuration options using `west config -l`:

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
# It is also possible to delete an option
# e.g., using `west config -d build.board`
```

> **Note:** The `west` configuration options can also be used to define arguments for CMake. This can be useful when debugging or when defining your own CMake caches, refer to the [documentation][zephyr-west-cmake-cfg] for details.

Now we can run `west build` in our current terminal without specifying a board.

```bash
$ west build -d ../build
```

Notice that CMake does not pick up `west`'s configuration options, and therefore executing `cmake -B ../build` would fail. One last parameter for `west build` that is worth mentioning is `--pristine`: Just like the `--pristine` option for `CMake`, instead of deleting the `build` folder, you can also use the `--pristine` to generate a new build:

```bash
$ west build -d ../build --pristine
```



## A quick glance at IDE integrations

In the [prerequisites](#prerequisites) we've promised that we won't have a detailed look at IDEs - and we won't. In this section, we'll just have a look `vscode` and some information that is relevant for any type of editor. Let's start with the editor-independent information:

Zephyr configures CMake to export a _compilation database_ and thus produces a `compile_commands.json` file in the build directory:

`zephyr/cmake/modules/kernel.cmake`
```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE CACHE BOOL
    "Export CMake compile commands. Used by gen_app_partitions.py script"
    FORCE
)
```

This compilation database essentially contains the commands that are used for each and every file that is compiled in the project. The database is not only used by tools such as [`clang-tidy`](https://clang.llvm.org/extra/clang-tidy), but also by IDEs and their extensions, e.g., to resolve include paths.

E.g., `vscode` has a [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools), which can be configured to pick up the provided compilation database as follows in `vscode`'s [`settings.json`](https://github.com/lmapii/practical-zephyr/blob/main/.vscode/settings.json):

`.vscode/settings.json`
```json
{
  "C_Cpp.default.compileCommands": "build/compile_commands.json",
}
```

If you like `vscode` and use [Nordic][nordicsemi]'s MCUs, you can also have a look at their [extension pack][nrf-connect-vscode], which comes with a graphical *devicetree* and *KConfig* extension, debugging support, and much more:

![nRF vscode Screenshot](../assets/nrf-vscode.png?raw=true)

> **Note:** Personally, I do not use the full extension pack since one key reason for using Zephyr is being able to use MCUs from _any_ supported vendor. I also tend to stick to the command line since I want to understand the process and do not rely on buttons in an IDE. Also, the extension pack somewhat conflicts with the very popular CMake extension [`ms-vscode.cmake-tools`](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools). But in general, [Nordic][nordicsemi] did a great job of providing an easy way to develop applications with their extension pack. My only hope would be, that they'll make the *Kconfig* and *devicetree* extensions usable without the full extension pack.

And that's it about IDE integrations! Ok, almost, we'll still see them every now and then in the article series.



## Flashing and debugging

Having built our first freestanding application, let's have a brief look at how we can flash and debug it on our target. We can do both using the `flash` and `debug` [extension commands][zephyr-west-cmd-ext-zephyr] provided by Zephyr for `west`. With the development kit plugged in, all we need to do is run `west flash` to program the MCU (the parameter `-d` is necessary to select the correct build directory in case it isn't located in the executing folder):

```bash
west flash -d ../build
-- west flash: rebuilding
ninja: no work to do.
-- west flash: using runner nrfjprog
Using board 123456789
-- runners.nrfjprog: Flashing file: ../build/zephyr/zephyr.hex
Parsing image file.
Verifying programming.
Verified OK.
Enabling pin reset.
Applying pin reset.
-- runners.nrfjprog: Board with serial number 123456789 flashed successfully.
```

> **Note:** In case you haven't followed the [installation](#installation) in this article but instead used your own setup, this step might not succeed: As you can see, `west` uses Nordic's [nRF Command Line Tool][nordicsemi-nrf-cmd] `nrfjprog` to program the development kit. In case this tool isn't available, the execution will fail. In general, flashing and debugging is always very target specific and you'll need to ensure that the correct tools are installed.

Wait, so how does Zephyr know which USB device to flash and _how_ to do this? `west` uses so-called [flash and debug **runners**][zephyr-west-cmd-ext-dbg]. In the above output, you can see that it selects Nordic's `nrfjprog` [command line tool][nordicsemi-nrf-cmd] as _runner_, which is in turn able to identify whether or not a board is connected to your computer. In case only a single matching board is present, it automatically programs that board.

The `west` extension command `debug` works in a similar fashion:

```bash
$ west debug -d ../build
-- west debug: rebuilding
ninja: no work to do.
-- west debug: using runner jlink
-- runners.jlink: JLink version: 7.66a
-- runners.jlink: J-Link GDB server running on port 2331; no thread info available
SEGGER J-Link GDB Server V7.66a Command Line Version

...

--Type <RET> for more, q to quit, c to continue without paging--
(gdb)
```

Here, `west` selected the J-Link runner since the development kit comes with a J-Link debugger. We end up with a `gdb` console which we can use to debug our application:

```bash
(gdb) break main
Breakpoint 1 at 0x4730: file /path/to/src/main.c, line 11.
(gdb) continue
Continuing.

Breakpoint 1, main () at /path/to/src/main.c:11
11              k_msleep(SLEEP_TIME_MS);
(gdb) next

Breakpoint 1, main () at /path/to/src/main.c:11
11              k_msleep(SLEEP_TIME_MS);
(gdb) next

Breakpoint 1, main () at /path/to/src/main.c:11
11              k_msleep(SLEEP_TIME_MS);
```

Debugging with `gdb` may be all you need, especially if you pair this with Python scripts.


### Debugging ARM Cortex-M targets in `vscode`

Aside from plain `gdb`, step debugging in your editor of choice is another very popular approach.

`vscode` is becoming more and more prominent, which is why it is worth mentioning that debugging solutions exist that allow step debugging within the editor, e.g., via the [`cortex-debug`](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) extension for ARM Cortex-M microcontrollers.

Setting up debugging in `vscode` is explained in the [official documentation](https://code.visualstudio.com/docs/editor/debugging), and comes down to providing launch configurations in the JSON file `.vscode/launch.json`. The following is an example file that can be used to debug Zephyr applications on boards using the nRF52840 microcontroller:

`.vscode/launch.json`
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "nRF52840_xxAA - attach",
      "device": "nRF52840_xxAA",
      "cwd": "${workspaceFolder}",
      "executable": "build/zephyr/zephyr.elf",
      "request": "launch",
      "type": "cortex-debug",
      "runToEntryPoint": "main",
      "servertype": "jlink",
      // in case you're sourcing "setup.sh" before launching vscode, you can use the environment variable:
      // "gdbPath": "${env:ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb"
      // otherwise, it is necessary to specify the full path to the gdb executable:
      "gdbPath": "/opt/nordic/ncs/toolchains/4ef6631da0/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb"
    }
  ]
}
```

> **Note:** Support for GDB comes out of the box with `vscode` and could also be used instead of relying on `cortex-debug`.

![nRF vscode Screenshot](../assets/vscode-debug.png?raw=true)

Your mileage, however, will always vary according to the MCU that you're using, and there is no generic solution. Zephyr's official documentation also comes with a dedicated [section about debugging](https://docs.zephyrproject.org/latest/develop/debug/index.html), and a separate section about [flash&debug host tools](https://docs.zephyrproject.org/latest/develop/flash_debug/host-tools.html). Pick your poison!

Another great resource about `vscode` with Zephyr is [Jonathan Beri's presentation "Zephyr & Visual Studio Code: How to Develop Zephyr Apps with a Modern, Visual IDE"][zephyr-ds-2023-zephyr-vscode] from the 2023 Zephyr Development Summit.



## Conclusion

In this article, we installed Zephyr and its tools using Nordic's [toolchain manager][nrf-connect-mgr], and with it have a base environment that we'll use in this series. Since your preference for development environments and installation for sure greatly varies, finding the matching solution will always be your own task. With the contents of the `setup.sh` script that we created in this article, you should have a good idea about what's involved when creating your own setup.

We then had a quick look at Zephyr's application types and created a skeleton _freestanding_ application, which we've compiled using CMake directly, and via Zephyr's meta-tool `west`. We've seen how we can parameterize our build to pick a certain _board_ and concluded, that it might be beneficial to use `west` over plain CMake - but you can always pick your preference.

We then brushed over the extension commands `flash` and `debug` that allow us to program the application to our board and to debug it using `gdb`. Finally, we've had a quick look at a possible `vscode` debugging integration.

Next up in line for this article series is _Kconfig_, which we briefly encountered when we created the minimum set of files for a freestanding application - which included a `prj.conf` file.



## Further reading

The following are great resources when it comes to Zephyr and are worth a read _or watch_:

- ["Zephyr and You: A Developer Environment for Newcomers"][zephyr-ds-2022-zephyr-and-you] by Lauren Murphy from the Zephyr Development Summit 2022.
- ["Zephyr & Visual Studio Code: How to Develop Zephyr Apps with a Modern, Visual IDE"][zephyr-ds-2023-zephyr-vscode] by Jonathan Beri from the Zephyr Development Summit 2023.
- [Zephyr's docker-image repository][zephyr-gh-docker] if you want to containerize your builds.
- [nRF Connect SDK Fundamentals][nrf-connect-sdk-fundamentals], which also/mostly cover Zephyr, in Nordic's [DevAcademy][nordicsemi-dev-academy].
- [Zephyr's own Getting Started Guide][zephyr-getting-started] in case you want to go down the rabbit hole yourself.
- ["Effective Modern CMake"][cmake-gist] to get you started with CMake fundamentals.
- The ["Getting Started with Zephyr"][billoo] series by Mohammed Billoo in case you're looking for an alternative series.
- [Zephyr notes][boseji] by Abhijit Boseji.
- [Benjamin Cabè's Blog][cabe], which includes a "Zephyr Weekly Update".

In general, I can always warmly recommend browsing through the videos from the _Zephyr Development Summit_, e.g., the playlists from the [2022][zephyr-ds-2022-playlist] and [2023][zephyr-ds-2023-playlist] Developers Summits.

Finally, have a look at the files in the [accompanying GitHub repository][practical-zephyr] and I hope you'll follow along with the future articles of this series!



<!-- References -->

[practical-zephyr]: https://github.com/lmapii/practical-zephyr

[billoo]: https://www.embeddedrelated.com/blogs-1/nf/Mohammed_Billoo.php
[boseji]: https://boseji.com/docs/zephyr/
[cabe]: https://blog.benjamin-cabe.com/

[vscode]: https://code.visualstudio.com/

[zephyr-ds-2022-playlist]: https://www.youtube.com/watch?v=o-f2qCd2AXo&list=PLzRQULb6-ipFDwFONbHu-Qb305hJR7ICe
[zephyr-ds-2023-playlist]: https://www.youtube.com/watch?v=PY64voxdhAU&list=PLzRQULb6-ipERkFrHaBh8tuSnK923ZUjY
[zephyr-ds-2023-zephyr-vscode]: https://www.youtube.com/watch?v=IKNHPmG-Qxo
[zephyr-ds-2022-zephyr-and-you]: https://www.youtube.com/watch?v=mQxh_UGA_Ik

[nordicsemi]: https://www.nordicsemi.com/
[nordicsemi-dev-academy]: https://academy.nordicsemi.com/
[nordicsemi-nrf52840-dk]: https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk
[nordicsemi-nrf-cmd]: https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools

[nrf-connect-sdk-fundamentals]: https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/
[nrf-connect-sdk]: https://www.nordicsemi.com/Products/Development-software/nrf-connect-sdk
[nrf-connect-mgr]: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/assistant.html#install-toolchain-manager
[nrf-connect-vscode]: https://nrfconnect.github.io/vscode-nrf-connect/
[nrf-vscode-kconfig]: https://marketplace.visualstudio.com/items?itemName=nordic-semiconductor.nrf-kconfig
[nrf-vscode-devicetree]: https://marketplace.visualstudio.com/items?itemName=nordic-semiconductor.nrf-devicetree

[git-env-sub]: https://git-scm.com/book/en/v2/Git-Internals-Environment-Variables
[git-env-tpl]: https://git-scm.com/docs/git-init#_template_directory
[git-submodules]: https://git-scm.com/book/en/v2/Git-Tools-Submodules

[cmake]: https://cmake.org/
[cmake-gist]: https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1

[zephyr-introduction]: https://docs.zephyrproject.org/latest/introduction/index.html
[zephyr-getting-started]: https://docs.zephyrproject.org/latest/develop/getting_started/index.html
[zephyr-boards]: https://docs.zephyrproject.org/latest/boards/index.html
[zephyr-west]: https://docs.zephyrproject.org/latest/develop/west/index.html
[zephyr-build-and-cfg-systems]: https://docs.zephyrproject.org/latest/build/index.html
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
[zephyr-kconfig]: https://docs.zephyrproject.org/latest/build/kconfig/index.html
[zephyr-west-cmd-ext]: https://docs.zephyrproject.org/latest/develop/west/zephyr-cmds.html
[zephyr-west-cmd-ext-zephyr]: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html
[zephyr-west-cmd-ext-dbg]: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html#flash-and-debug-runners
[zephyr-west-build-cfg]: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html#configuration-options
[zephyr-nothread]: https://docs.zephyrproject.org/latest/kernel/services/threads/nothread.html#nothread
[zephyr-cmake-pkg]: https://docs.zephyrproject.org/latest/build/zephyr_cmake_package.html
[zephyr-cmake-pkg-source]: https://docs.zephyrproject.org/latest/build/zephyr_cmake_package.html#zephyr-cmake-package-source-code
[zephyr-cmakelists-app]: https://docs.zephyrproject.org/latest/develop/application/index.html#application-cmakelists-txt
[zephyr-west-builtin-cfg]: https://docs.zephyrproject.org/latest/develop/west/config.html#built-in-configuration-options
[zephyr-west-cmake-cfg]: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html#permanent-cmake-arguments

[zephyr-gh-docker]: https://github.com/zephyrproject-rtos/docker-image
