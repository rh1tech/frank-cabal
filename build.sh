#!/bin/bash
# Build script for Cabal (ScummVM port for RP2350)
#
# Usage: ./build.sh [BOARD] [CPU_MHZ] [PSRAM_MHZ] [FLASH_MHZ] [usb-hid] [clean]
# Defaults: M2 504 133 66

set -e

USB_HID="0"
CLEAN=""

# Strip special (non-positional) flags so they don't pollute the positional
# slots for BOARD/CPU/PSRAM/FLASH.
POS=()
for arg in "$@"; do
    case "$arg" in
        clean) CLEAN="clean" ;;
        usb-hid|usbhid) USB_HID="1" ;;
        *) POS+=("$arg") ;;
    esac
done

BOARD="${POS[0]:-M2}"
CPU="${POS[1]:-504}"
PSRAM="${POS[2]:-133}"
FLASH="${POS[3]:-66}"

# Validate board variant
if [[ "$BOARD" != "M1" && "$BOARD" != "M2" ]]; then
    echo "Invalid board variant: $BOARD"
    echo "Usage: $0 [M1|M2] [CPU_MHZ] [PSRAM_MHZ] [FLASH_MHZ] [usb-hid] [clean]"
    echo "  CPU_MHZ:   252, 378, 504  (default: 504)"
    echo "  PSRAM_MHZ: 84, 100, 133, 166  (default: 133)"
    echo "  FLASH_MHZ: flash QMI cap in MHz  (default: 66)"
    echo "  usb-hid:   Enable USB keyboard/mouse (disables USB serial, uses UART)"
    exit 1
fi

echo "Building Cabal:"
echo "  Board: $BOARD"
echo "  CPU:   $CPU MHz"
echo "  PSRAM: $PSRAM MHz"
echo "  Flash: $FLASH MHz"
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
      -DFLASH_SPEED="$FLASH" \
      -DUSB_HID_ENABLED="$USB_HID" \
      ..

# Build
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo "  Firmware: build/frank-cabal.uf2"
if [[ -f frank-cabal.uf2 ]]; then
    echo "  Size: $(ls -lh frank-cabal.uf2 | awk '{print $5}')"
fi
