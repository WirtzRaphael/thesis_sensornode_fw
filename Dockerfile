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
   
# Install Pico SDK
RUN \
    mkdir -p /project/src/ && \
    mkdir -p /project/McuLib && \
    mkdir -p /project/build && \
    cd /project/ && \
    git clone https://github.com/raspberrypi/pico-sdk.git --branch master && \
    cd pico-sdk/ && \
    git submodule update --init && \
    cd /

# Set the Pico SDK environment variable
ENV PICO_SDK_PATH=/project/pico-sdk/

# Install McuLib
# alternative : setup script
COPY McuLib/ /project/McuLib/

# Source files
COPY src/* /project/src/

# Build files and setup
COPY CMakeLists.txt /project/
COPY run_build.sh /project/
COPY run_init.sh /project/

RUN \
    cd /project/ && \
    ./run_init.sh

# Build project
RUN \
    cd /project/ && \
    ./run_build.sh
    
# Command that will be invoked when the container starts
ENTRYPOINT ["/bin/bash"]