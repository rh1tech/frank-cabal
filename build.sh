#!/bin/bash
# Build script for Cabal (ScummVM port for RP2350)
# Usage: ./build.sh [BOARD] [CPU_MHZ] [PSRAM_MHZ] [usb-hid] [clean]
# Defaults: M1 378 133

set -e

BOARD="${1:-M1}"
CPU="${2:-378}"
PSRAM="${3:-133}"
USB_HID="0"
CLEAN=""

# Handle special arguments
for arg in "$@"; do
    if [[ "$arg" == "clean" ]]; then
        CLEAN="clean"
    fi
    if [[ "$arg" == "usb-hid" || "$arg" == "usbhid" ]]; then
        USB_HID="1"
    fi
done

# Validate board variant
if [[ "$BOARD" != "M1" && "$BOARD" != "M2" ]]; then
    echo "Invalid board variant: $BOARD"
    echo "Usage: $0 [M1|M2] [CPU_MHZ] [PSRAM_MHZ] [usb-hid] [clean]"
    echo "  CPU_MHZ: 252, 378, 504 (default: 378)"
    echo "  PSRAM_MHZ: 84, 100, 133, 166 (default: 133)"
    echo "  usb-hid: Enable USB keyboard/mouse (disables USB serial, uses UART)"
    exit 1
fi

echo "Building Cabal:"
echo "  Board: $BOARD"
echo "  CPU: $CPU MHz"
echo "  PSRAM: $PSRAM MHz"
if [[ "$USB_HID" == "1" ]]; then
    echo "  Input: USB HID keyboard/mouse (UART console)"
else
    echo "  Input: PS/2 keyboard/mouse (USB serial console)"
fi
echo "  Audio: I2S"
echo ""

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
      -DBOARD_VARIANT="$BOARD" \
      -DCPU_SPEED="$CPU" \
      -DPSRAM_SPEED="$PSRAM" \
      -DUSB_HID_ENABLED="$USB_HID" \
      ..

# Build
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo "  Firmware: build/cabal.uf2"
echo "  Size: $(ls -lh cabal.uf2 | awk '{print $5}')"
