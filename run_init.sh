#!/bin/bash

# - generate files with build flags for clangd
cmake -G "Ninja" . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1