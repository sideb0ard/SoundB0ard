# Building SoundB0ard

SoundB0ard is a command-line music making environment that can be built on Linux, macOS, and other Unix-like systems.

## Quick Start

1. **Install dependencies** (run the provided script):
   ```bash
   ./install_deps.sh
   ```

2. **Build the project**:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . -j$(nproc)
   ```

3. **Install** (optional):
   ```bash
   sudo cmake --build . --target install
   ```

## Manual Dependency Installation

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git \
    portaudio19-dev libportmidi-dev \
    libsndfile1-dev libreadline-dev \
    libjsoncpp-dev \
    pkg-config
```

### macOS (with Homebrew)
```bash
brew install \
    portaudio portmidi libsndfile \
    readline jsoncpp pkg-config
```

### Arch Linux
```bash
sudo pacman -S --needed \
    base-devel cmake git \
    portaudio portmidi libsndfile \
    readline jsoncpp pkgconf
```

### Red Hat/CentOS/Fedora
```bash
sudo yum install -y \
    gcc-c++ cmake git \
    portaudio-devel portmidi-devel \
    libsndfile-devel readline-devel \
    jsoncpp-devel \
    pkgconfig
```

## Dependencies

SoundB0ard uses the following libraries:

### Automatically Downloaded (via FetchContent)
- **Ableton Link** - For tempo synchronization
- **PerlinNoise** - For procedural noise generation

### System Libraries (must be installed)
- **PortAudio** - Cross-platform audio I/O
- **PortMidi** - Cross-platform MIDI I/O  
- **libsndfile** - Audio file reading/writing
- **Readline** - Command-line editing
- **JsonCpp** - JSON parsing
- **pkg-config** - Package configuration

## Build Options

- **Debug build**: `cmake -DCMAKE_BUILD_TYPE=Debug ..`
- **With tests**: `cmake -DBUILD_TESTS=ON ..`
- **With sanitizers**: `cmake -DENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug ..`

## Development Targets

- `cmake --build . --target format` - Format code with clang-format
- `cmake --build . --target cppcheck` - Run static analysis  
- `cmake --build . --target lint` - Run all linting tools
- `cmake --build . --target dev` - Quick development cycle (format + build)
- `cmake --build . --target check-all` - Run all checks

## Troubleshooting

- **CMake fails to find libraries**: Install the development packages for your distribution
- **Link errors**: Make sure all libraries are installed with their development headers
- **Permission denied**: Run `chmod +x install_deps.sh` to make the script executable