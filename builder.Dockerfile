# This is just a demo setup of the environment tailored to the nRF SDK.
# The nRF SDK is not necessary to follow along the examples in this repository, but it is
# recommended, since the examples also use an nRF development kit for some "hands on".
#
# This Dockerfile also fully initializes Zephyr for use with freestanding applications, which
# isn't something you'd typically do with a workspace application. Instead, it is recommended
# to rely on the files provided by Zephyr (or the vendor), e.g.,
# https://github.com/zephyrproject-rtos/docker-image

ARG base_tag=bullseye
ARG base_img=mcr.microsoft.com/vscode/devcontainers/base:dev-${base_tag}
# ARG base_img=debian:${base_tag}

FROM --platform=linux/amd64 ${base_img} AS builder-install

RUN apt-get update --fix-missing && apt-get -y upgrade
RUN apt-get install -y --no-install-recommends \
    apt-utils \
    build-essential \
    curl \
    cmake \
    git \
    gperf \
    libfdt1 \
    libncurses5 \
    libncurses5-dev \
    libusb-1.0-0-dev  \
    libyaml-dev \
    locales \
    make \
    ninja-build \
    python3-pip \
    python3-setuptools \
    udev \
    unzip \
    wget \
    && apt-get -y clean \
    && apt-get -y autoremove \
    && rm -rf /var/lib/apt/lists/*

ENV LANG='en_US.UTF-8' LANGUAGE='en_US:en' LC_ALL='en_US.UTF-8'

RUN echo 'en_US.UTF-8 UTF-8' > /etc/locale.gen && /usr/sbin/locale-gen
RUN echo "alias ll='ls -laGFh'" >> /root/.bashrc

VOLUME ["/workspaces/practical-zephyr"]
WORKDIR /workspaces/practical-zephyr

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# install ARM gcc toolchain

FROM builder-install AS builder-arm-gcc

RUN wget -O arm-gcc.tgz "https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2?revision=108bd959-44bd-4619-9c19-26187abf5225&la=en&hash=E788CE92E5DFD64B2A8C246BBA91A249CB8E2D2D" && \
    tar xvfj arm-gcc.tgz -C /opt && \
    rm arm-gcc.tgz

RUN /opt/gcc-arm-none-eabi-9-2019-q4-major/bin/arm-none-eabi-gcc --version

# instead of installing the toolchain manually, the Zephyr SDK could be used:
# https://github.com/zephyrproject-rtos/sdk-ng

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# update python packages

FROM builder-arm-gcc AS builder-pip

# Latest PIP & Python dependencies
RUN python3 -m pip install -U pip && \
    python3 -m pip install -U setuptools && \
    python3 -m pip install cmake>=3.20.0 wheel && \
    python3 -m pip install -U west>=1.0

# Newer PIP will not overwrite distutils, so upgrade PyYAML manually
RUN python3 -m pip install --ignore-installed -U PyYAML

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# download nRF SDK and west dependencies to install pip requirements

FROM builder-pip AS builder-sdk
ARG sdk_nrf_revision=v2.5.0

# install devicetree compiler
RUN apt-get update --fix-missing && apt-get -y upgrade
RUN apt-get install -y --no-install-recommends \
    device-tree-compiler>=1.5.1 \
    && apt-get -y clean \
    && apt-get -y autoremove \
    && rm -rf /var/lib/apt/lists/*

# install the nRF SDK, which includes Zephyr.
WORKDIR /workspaces
RUN west init -m https://github.com/nrfconnect/sdk-nrf --mr ${sdk_nrf_revision} sdk-nrf && \
    cd sdk-nrf && west update --narrow -o=--depth=1

WORKDIR /workspaces/sdk-nrf
RUN python3 -m pip install -r zephyr/scripts/requirements.txt && \
    python3 -m pip install -r nrf/scripts/requirements.txt && \
    python3 -m pip install -r bootloader/mcuboot/scripts/requirements.txt

ENV ZEPHYR_BASE=/workspaces/sdk-nrf/zephyr
ENV PATH="${ZEPHYR_BASE}/scripts:${PATH}"

# https://docs.zephyrproject.org/latest/develop/toolchains/gnu_arm_embedded.html
ENV ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
ENV GNUARMEMB_TOOLCHAIN_PATH=/opt/gcc-arm-none-eabi-9-2019-q4-major
ENV PATH="${GNUARMEMB_TOOLCHAIN_PATH}/bin:${PATH}"

ENV XDG_CACHE_HOME=/workspaces/.cache

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# install invoke

RUN python3 -m pip install invoke
