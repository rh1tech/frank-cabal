#!/bin/bash
# Build script for Cabal (ScummVM port for RP2350)
# Usage: ./build.sh [M1|M2] [clean]

BOARD_VARIANT="${1:-M1}"
CLEAN="${2:-}"

# Validate board variant
if [[ "$BOARD_VARIANT" != "M1" && "$BOARD_VARIANT" != "M2" ]]; then
    echo "Invalid board variant: $BOARD_VARIANT"
    echo "Usage: $0 [M1|M2] [clean]"
    exit 1
fi

echo "Building Cabal for $BOARD_VARIANT..."

# Clean if requested
if [[ "$CLEAN" == "clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf ./build
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake -DPICO_PLATFORM=rp2350 \
      -DBOARD_VARIANT="$BOARD_VARIANT" \
      -DUSB_HID_ENABLED=0 \
      ..

# Build
make -j4

echo ""
echo "Build complete!"
echo "Firmware: build/cabal.elf"
echo "UF2: build/cabal.uf2"
