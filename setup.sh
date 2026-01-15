#!/bin/sh

ncs_install_dir="${ncs_install_dir:-/opt/nordic/ncs}"
ncs_sdk_version="${ncs_sdk_version:-v3.2.1}"
ncs_bin_version="${ncs_bin_version:-322ac893fe}"

shell_name=$(basename "$SHELL")

paths=(
    $ncs_install_dir/toolchains/$ncs_bin_version/bin
    $ncs_install_dir/toolchains/$ncs_bin_version/opt/nanopb/generator-bin
)

# Extend the path with the binaries provided by Nordic's installation.
#
# Make sure to _append_ to the path and don't _prepend_ in case you want to use your
# own executables, e.g., for `git`. This is important if you'd like to use `west init`,
# since at the time of writing Nordic doesn't deliver the 'remote-https' binary for git
# and thus running `west init` would fail with an error like the following:
#
# $ west init west-playground
# === Initializing in /path/to/west-playground
# --- Cloning manifest repository from https://github.com/zephyrproject-rtos/zephyr
# Cloning into '/path/to/west-playground/.west/manifest-tmp'...
# warning: templates not found in /opt/homebrew/Cellar/git/2.37.3/share/git-core/templates
# git: 'remote-https' is not a git command. See 'git --help'.
# FATAL ERROR: command exited with status 128: git clone https://github.com/zephyrproject-rtos/zephyr /path/to/west-playground/.west/manifest-tmp
#
for entry in ${paths[@]}; do
    export PATH=$PATH:$entry
done

# use the nrfutil installation from the SDK installation
# yep, nrfutil installs itself, and with the required extension commands
export NRFUTIL_HOME=$ncs_install_dir/toolchains/$ncs_bin_version/nrfutil/home

export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=$ncs_install_dir/toolchains/$ncs_bin_version/opt/zephyr-sdk
source $ncs_install_dir/$ncs_sdk_version/zephyr/zephyr-env.sh
west zephyr-export

# west completion is also available:
# https://docs.zephyrproject.org/latest/develop/west/zephyr-cmds.html
# use `west completion $shell_name`
