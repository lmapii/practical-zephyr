# This is just a demo setup of the environment tailored to the nRF SDK.
# The nRF SDK is not necessary to follow along the examples in this repository, but it is
# recommended, since the examples also use an nRF development kit for some "hands on".
#
# This Dockerfile also fully initializes Zephyr for use with freestanding applications, which
# isn't something you'd typically do with a workspace application. Instead, it is recommended
# to rely on the files provided by Zephyr (or the vendor), e.g.,
# https://github.com/zephyrproject-rtos/docker-image

# DISCLAIMER: It is also highly recommended to simply _not_ use out-of-tree (freestanding)
# applications since this assumes that Zephyr is installed someplace. With images, this
# is no fun since you're getting into all kinds of user permission mismatches for things
# that are cloned into an image.

ARG base_tag=trixie
ARG base_img=mcr.microsoft.com/devcontainers/base:dev-${base_tag}
# ARG base_img=debian:${base_tag}

FROM --platform=linux/amd64 ${base_img} AS builder-install

ENV LANG='en_US.UTF-8' LANGUAGE='en_US:en' LC_ALL='en_US.UTF-8'
ENV PYTHONDONTWRITEBYTECODE=1

RUN apt-get update --fix-missing && apt-get -y upgrade
RUN apt-get install -y --no-install-recommends \
    apt-utils \
    build-essential \
    curl \
    cmake \
    device-tree-compiler>=1.5.1 \
    git \
    gperf \
    locales \
    make \
    ninja-build \
    python3-pip \
    python3-setuptools \
    python3-venv \
    wget \
    && apt-get -y clean \
    && apt-get -y autoremove \
    && rm -rf /var/lib/apt/lists/*

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# install ARM gcc toolchain

FROM builder-install AS builder-arm-gcc

RUN wget -O arm-gcc.tar.xz \
    https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz && \
    tar -xJf arm-gcc.tar.xz -C /opt && \
    rm arm-gcc.tar.xz
RUN /opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc --version

# instead of installing the toolchain manually, the Zephyr SDK could be used.
# however, the `sdk-nrf` doesn't come with the "west sdk-install" extension command.
# therefore, we're sticking with the ARM toolchain for now.
# https://github.com/zephyrproject-rtos/sdk-ng

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# update python packages

FROM builder-arm-gcc AS builder-pip

# Set default shell during Docker image build to bash
SHELL ["/bin/bash", "-eo", "pipefail", "-c"]

ARG USERNAME=user
ARG UID=1000
ARG GID=1000
ARG PYTHON_VENV_PATH=/opt/python/venv

RUN <<EOF
    userdel -r vscode || true
    groupadd -g $GID -o $USERNAME
    useradd -u $UID -m -g $USERNAME -G plugdev $USERNAME
    echo $USERNAME ' ALL = NOPASSWD: ALL' > /etc/sudoers.d/$USERNAME
    chmod 0440 /etc/sudoers.d/$USERNAME
EOF

RUN echo 'en_US.UTF-8 UTF-8' > /etc/locale.gen && /usr/sbin/locale-gen

RUN <<EOF
    mkdir -p ${PYTHON_VENV_PATH}
    python3 -m venv ${PYTHON_VENV_PATH}
    source ${PYTHON_VENV_PATH}/bin/activate

    python3 -m pip install --no-cache-dir -U pip
    python3 -m pip install --no-cache-dir -U setuptools
    python3 -m pip install --no-cache-dir cmake>=3.20.0 wheel
    python3 -m pip install --no-cache-dir -U west>=1.0

    # optional, but used by CI
    python3 -m pip install --no-cache-dir invoke

    # Newer PIP will not overwrite distutils, so upgrade PyYAML manually
    python3 -m pip install --no-cache-dir --ignore-installed -U PyYAML
EOF

ENV PATH=${PYTHON_VENV_PATH}/bin:$PATH

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# download nRF SDK and west dependencies to install pip requirements

FROM builder-pip AS builder-sdk
ARG sdk_nrf_revision=v3.2.1

# unless you're building _and running_ this image with the same user there is no way to guarantee
# that the numeric UID ownership of the repositories cloned by `west init` are the same as your
# user's UIDs. this leads to build failures since building Zephyr projects invokes git commands
# within the repository that is in ZEPHYR_BASE. therefore and for images only, we disable
# ownership checks for git.
RUN git config --system safe.directory '*'

# install the nRF SDK, which includes Zephyr.
# running out-of-tree builds also manipulates `/workspaces/sdk-nrf/.west/config`
# so we'll give everyone access permission to the workspace. notice that we keep `chmod` in the
# same RUN instruction to avoid (massively) wasting space due to Docker's layers
# see https://garbers.co.za/2017/11/15/reduce-docker-image-size-chmod-chown/
WORKDIR /workspaces
RUN west init -m https://github.com/nrfconnect/sdk-nrf --mr ${sdk_nrf_revision} -o=--depth=1 sdk-nrf && \
    cd sdk-nrf && \
    west update --narrow -o=--depth=1 && \
    chmod -R 777 /workspaces/sdk-nrf

WORKDIR /workspaces/sdk-nrf
RUN <<EOF
    python3 -m pip install --no-cache-dir -r zephyr/scripts/requirements.txt
    python3 -m pip install --no-cache-dir -r nrf/scripts/requirements.txt

    # we're not using any bootloaders in this demo, let's save some space
    # python3 -m pip install --no-cache-dir -r bootloader/mcuboot/scripts/requirements.txt
EOF

ENV ZEPHYR_BASE=/workspaces/sdk-nrf/zephyr
ENV PATH="${ZEPHYR_BASE}/scripts:${PATH}"

# https://docs.zephyrproject.org/latest/develop/toolchains/gnu_arm_embedded.html
ENV ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
ENV GNUARMEMB_TOOLCHAIN_PATH=/opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi
ENV PATH="${GNUARMEMB_TOOLCHAIN_PATH}/bin:${PATH}"

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# switch to 'user'

FROM builder-sdk AS builder

ENV XDG_CACHE_HOME=/tmp/.cache
RUN mkdir -p /tmp/.cache && \
    chmod 777 /tmp/.cache && \
    chown -R user:user /tmp/.cache

USER user
RUN echo "alias ll='ls -laGFh'" >> ~/.bashrc

WORKDIR /workspaces/practical-zephyr
