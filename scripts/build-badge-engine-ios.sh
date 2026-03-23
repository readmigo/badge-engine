#!/bin/bash
set -euo pipefail

# Build badge-engine static library for iOS and copy to Readmigo iOS Vendor directory.
# Usage: ./scripts/build-badge-engine-ios.sh [--with-filament]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
IOS_VENDOR_DIR="$(cd "$ENGINE_DIR/../ios/Readmigo/Vendor/BadgeEngine" 2>/dev/null && pwd || echo "")"

USE_FILAMENT=OFF
if [[ "${1:-}" == "--with-filament" ]]; then
    USE_FILAMENT=ON
fi

echo "=== badge-engine iOS build ==="
echo "Engine dir: $ENGINE_DIR"
echo "Filament: $USE_FILAMENT"

# Build for device (arm64)
echo ""
echo "--- Building for iOS device (arm64) ---"
cmake -B "$ENGINE_DIR/build-ios-device" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES="arm64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
    -DBADGE_ENGINE_BUILD_TESTS=OFF \
    -DBADGE_ENGINE_USE_FILAMENT=$USE_FILAMENT \
    -S "$ENGINE_DIR"

cmake --build "$ENGINE_DIR/build-ios-device" --config Release

DEVICE_LIB="$ENGINE_DIR/build-ios-device/libbadge_engine.a"
if [[ ! -f "$DEVICE_LIB" ]]; then
    echo "ERROR: Device library not found at $DEVICE_LIB"
    exit 1
fi
echo "Device library: $(du -h "$DEVICE_LIB" | cut -f1)"

# Copy to Vendor
if [[ -n "$IOS_VENDOR_DIR" ]]; then
    echo ""
    echo "--- Copying to iOS Vendor ---"
    mkdir -p "$IOS_VENDOR_DIR/lib"
    mkdir -p "$IOS_VENDOR_DIR/include/badge_engine"

    cp "$DEVICE_LIB" "$IOS_VENDOR_DIR/lib/libbadge_engine.a"
    cp "$ENGINE_DIR/include/badge_engine/badge_engine.h" "$IOS_VENDOR_DIR/include/badge_engine/"
    cp "$ENGINE_DIR/include/badge_engine/types.h" "$IOS_VENDOR_DIR/include/badge_engine/"

    # Update VERSION
    COMMIT=$(cd "$ENGINE_DIR" && git rev-parse --short HEAD 2>/dev/null || echo "unknown")
    DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    cat > "$IOS_VENDOR_DIR/VERSION" <<EOF
version=v0.1.0
commit=$COMMIT
date=$DATE
EOF

    echo "Copied to: $IOS_VENDOR_DIR"
    echo "Headers: $(ls "$IOS_VENDOR_DIR/include/badge_engine/")"
    echo "Library: $(du -h "$IOS_VENDOR_DIR/lib/libbadge_engine.a" | cut -f1)"
else
    echo "WARNING: iOS Vendor directory not found, skipping copy"
    echo "Library available at: $DEVICE_LIB"
fi

echo ""
echo "=== Done ==="
