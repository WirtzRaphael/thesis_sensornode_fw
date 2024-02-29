#!/bin/bash
REPO_FOLDER=mcuoneclipse
LIB_FOLDER=../McuLib

# Navigate to project root folder

# Remove existing repository
if [ -d "$REPO_FOLDER" ]; then
    echo "Remove $REPO_FOLDER"
    rm -rf "$REPO_FOLDER"
fi

# Remove existing library
if [ -d "$LIB_FOLDER" ]; then
    echo "Remove $LIB_FOLDER"
    rm -rf "$LIB_FOLDER"
fi

# Clone library from the repsitory
git clone -n --depth=1 --filter=tree:0 git@github.com:ErichStyger/McuOnEclipseLibrary.git $REPO_FOLDER
cd $REPO_FOLDER
git sparse-checkout set --no-cone lib
git checkout

# Copy files to library folder
cp -r lib ../$LIB_FOLDER

# Cleanup repository files
if [ -d "$REPO_FOLDER" ]; then
    echo "Remove $REPO_FOLDER"
    rm -rf "$REPO_FOLDER"
fi
