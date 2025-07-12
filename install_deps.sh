#!/bin/bash

# SoundB0ard Dependency Installation Script
# This script helps install the required system dependencies for building SoundB0ard

set -e

echo "Installing SoundB0ard dependencies..."

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    if command -v apt-get &> /dev/null; then
        # Ubuntu/Debian
        echo "Detected Ubuntu/Debian system"
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            git \
            portaudio19-dev \
            libportmidi-dev \
            libsndfile1-dev \
            libreadline-dev \
            libjsoncpp-dev \
            pkg-config
    elif command -v yum &> /dev/null; then
        # Red Hat/CentOS/Fedora
        echo "Detected Red Hat/CentOS/Fedora system"
        sudo yum install -y \
            gcc-c++ \
            cmake \
            git \
            portaudio-devel \
            portmidi-devel \
            libsndfile-devel \
            readline-devel \
            jsoncpp-devel \
            pkgconfig
    elif command -v pacman &> /dev/null; then
        # Arch Linux
        echo "Detected Arch Linux system"
        sudo pacman -S --needed \
            base-devel \
            cmake \
            git \
            portaudio \
            portmidi \
            libsndfile \
            readline \
            jsoncpp \
            pkgconf
    else
        echo "Unsupported Linux distribution. Please install the following packages manually:"
        echo "- build tools (gcc, cmake, git)"
        echo "- portaudio development files"
        echo "- portmidi development files"
        echo "- libsndfile development files"
        echo "- readline development files"
        echo "- jsoncpp development files"
        echo "- pkg-config"
        exit 1
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "Detected macOS system"
    if command -v brew &> /dev/null; then
        brew install \
            portaudio \
            portmidi \
            libsndfile \
            readline \
            jsoncpp \
            pkg-config
    else
        echo "Homebrew not found. Please install Homebrew first:"
        echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi
else
    echo "Unsupported operating system: $OSTYPE"
    exit 1
fi

echo "Dependencies installed successfully!"
echo ""
echo "To build SoundB0ard:"
echo "  mkdir -p build"
echo "  cd build"
echo "  cmake .."
echo "  cmake --build . -j\$(nproc)"
echo ""
echo "To install SoundB0ard:"
echo "  sudo cmake --build . --target install"