#!/bin/bash

# OceanRoute Nav - Linux Build Script
# -----------------------------------

set -e  # Exit on error

echo "🚢 OceanRoute Nav - Build Script"
echo "================================"

# Check for SFML
echo "Checking for SFML..."
if ! pkg-config --exists sfml-graphics 2>/dev/null; then
    echo "❌ SFML not found!"
    echo ""
    echo "Please install SFML:"
    echo "  Ubuntu/Debian: sudo apt-get install libsfml-dev"
    echo "  Fedora:        sudo dnf install SFML-devel"
    echo "  Arch Linux:    sudo pacman -S sfml"
    exit 1
fi
echo "✅ SFML found"

# Check for g++
echo "Checking for g++..."
if ! command -v g++ &> /dev/null; then
    echo "❌ g++ not found!"
    echo "Please install build-essential or gcc-c++"
    exit 1
fi
echo "✅ g++ found"

# Get SFML flags
SFML_CFLAGS=$(pkg-config --cflags sfml-graphics sfml-window sfml-system 2>/dev/null || echo "")
SFML_LIBS=$(pkg-config --libs sfml-graphics sfml-window sfml-system 2>/dev/null || echo "-lsfml-graphics -lsfml-window -lsfml-system")

# Build
echo ""
echo "🔨 Building OceanRoute Nav..."
g++ -std=c++11 src/main.cpp -o OceanRouteNav $SFML_CFLAGS $SFML_LIBS

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "Run with: ./OceanRouteNav"
else
    echo ""
    echo "❌ Build failed!"
    exit 1
fi
