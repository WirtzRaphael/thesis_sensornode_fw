#!/bin/bash

# - generate files with build flags for clangd
cmake -G "Ninja" . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1