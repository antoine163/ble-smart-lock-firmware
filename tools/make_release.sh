#!/bin/bash

# Check the number of arguments
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <VERSION>"
    exit 1
fi

# Set the VERSION variable
VERSION="$1"

# Check if the version is valid
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Invalid version format. Please use the format x.y.z."
    exit 1
fi

# Path to the project directory
PROJECT_DIR="$PWD/.."

# Compile for the specified version
echo "Compiling for version $VERSION..."
mkdir -p "build"
cd "build"
cmake --toolchain "$PROJECT_DIR/cmake/arm-none-eabi-gcc.cmake" -DMODEL_BLUENRG=M2SA -DVERSION="$VERSION" -DCMAKE_BUILD_TYPE=Release "$PROJECT_DIR"
cmake --build .
cmake --install .
cmake --toolchain "$PROJECT_DIR/cmake/arm-none-eabi-gcc.cmake" -DMODEL_BLUENRG=M2SP -DVERSION="$VERSION" -DCMAKE_BUILD_TYPE=Release "$PROJECT_DIR"
cmake --build .
cmake --install .

echo "Compilation completed."
