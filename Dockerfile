# Fetch ubuntu image
FROM ubuntu:22.04 AS base

# Install prerequisites
RUN \
    apt update && \
    apt install -y \
    git

RUN \
    apt install -y \
    python3 \ 
    cmake \ 
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    build-essential \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*
   
# Setup project directory
RUN \
    mkdir -p /project/src/ && \
    mkdir -p /project/McuLib && \
    mkdir -p /project/build

# Source files
COPY src/* /project/src/

# Build files and setup
COPY CMakeLists.txt /project/
COPY run_build.sh /project/
COPY run_init.sh /project/
COPY setup/mculib.sh /project/setup

# Library : McuLib
# - alternative : setup script
COPY McuLib/ /project/McuLib/

# Library : Pico SDK
RUN \
    cd /project/ && \
    git clone https://github.com/raspberrypi/pico-sdk.git --branch master && \
    cd pico-sdk/ && \
    git submodule update --init && \
    cd /

ENV PICO_SDK_PATH=/project/pico-sdk/

# Library : Pico-Extras
RUN \
    cd /project/ && \
    git clone https://github.com/raspberrypi/pico-extras.git --branch master

ENV PICO_EXTRAS_PATH=/project/pico-extras/

RUN ls -l /project/


# Configure CMake project
RUN cd /project/ && \
    cmake -G "Ninja" . -B build
# RUN cd /project/ && \
#     chmod +x run_init.sh \
#     ./run_init.sh

# Build project
RUN cd /project/ && \
    cmake --build build
# RUN cd /project/ && \
#     chmod +x run_build.sh && \
#     ./run_build.sh
    
# Command that will be invoked when the container starts
ENTRYPOINT ["/bin/bash"]