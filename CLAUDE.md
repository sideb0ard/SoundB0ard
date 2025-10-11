# SoundB0ard Project Context

## Project Overview

SoundB0ard is an interactive music-making environment with a Unix-style shell interface. It's a real-time audio synthesis and sampling system that allows users to create music through command-line interactions.

### Key Features
- Unix-style shell for music creation and performance
- Ableton Link integration for tempo synchronization across apps
- Real-time audio synthesis with multiple synth engines
- Drum synthesis and sampling capabilities
- Granular synthesis
- Pattern-based sequencing
- Audio effects processing
- MIDI support
- WebSocket integration for remote control
- Live coding support - can track files for changes 

## Technology Stack

- **Language**: C++20
- **Build System**: CMake
- **Platform**: macOS (primary), Linux (cross-platform capable)
- **Audio I/O**: PortAudio
- **MIDI I/O**: PortMidi
- **File I/O**: libsndfile
- **CLI**: readline
- **Sync**: Ableton Link
- **JSON**: JsonCpp
- **WebSockets**: websocketpp

## Architecture

### Core Components

- **AudioEngine** (`AudioEngine.cpp/hpp`): Main audio processing engine
- **AudioPlatform** (`AudioPlatform.cpp/hpp`): Platform-specific audio handling
- **Mixer** (`mixer.cpp/h`): Audio mixing and routing
- **Process** (`process.cpp/hpp`): Audio processing pipeline

### Synthesis Engines

- **DXSynth** (`dxsynth.cpp/h`): FM-style synthesis engine
- **MiniSynth** (`minisynth.cpp/h`): Subtractive synthesis engine
- **SBSynth** (`sbsynth.cpp/h`): Custom synthesis engine
- **DrumSynth** (`drum_synth.cpp/h`): Drum synthesis
- **DrumSampler** (`drumsampler.cpp/h`): Sample-based drums

### Modulation & Control

- **Envelope Generator** (`envelope_generator.cpp/h`): ADSR envelopes
- **LFO** (`lfo.cpp/h`): Low-frequency oscillators
- **ModMatrix** (`modmatrix.cpp/h`): Modulation routing matrix
- **Stepper** (`stepper.cpp/h`): Step sequencer

### Audio Processing

- **Filters** (`filters/`): Various filter implementations
- **FX** (`fx/`): Audio effects processors
- **Granulator** (`granulator.cpp/h`): Granular synthesis
- **Oscillators**: Multiple oscillator types (bandlimited, wavetable)

### I/O & Control

- **CmdLoop** (`cmdloop.cpp/h`): Main command loop and shell
- **Interpreter** (`interpreter/`): Command parsing and execution
- **MIDIDevice** (`midi_device.cpp/h`): MIDI input/output handling
- **WebSocket** (`websocket/`): Network control interface

### Utilities

- **FileBuffer/FileReader**: Audio file loading
- **Utils**: General utility functions
- **AudioUtils**: Audio-specific utilities
- **PatternParser** (`pattern_parser/`): Pattern language parsing

## Build & Development

### Building
```bash
./install_deps.sh    # Install dependencies
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Development Targets
- `cmake --build . --target format` - Format code with clang-format
- `cmake --build . --target cppcheck` - Static analysis
- `cmake --build . --target lint` - All linting tools
- `cmake --build . --target dev` - Quick dev cycle (format + build)
- `cmake --build . --target check-all` - All checks

### Running
```bash
# Run from project root (so it can find wavs/ directory)
build/Sbsh
```

## Code Style & Conventions

- **Standard**: C++20
- **Formatting**: clang-format (config in `.clang-format`)
- **Linting**: clang-tidy (config in `.clang-tidy`)
- **Pre-commit hooks**: Configured in `.pre-commit-config.yaml`

## Project Structure

```
SoundB0ard/
├── src/              # Source code
│   ├── filters/      # Filter implementations
│   ├── fx/           # Effects processors
│   ├── interpreter/  # Command interpreter
│   ├── pattern_parser/ # Pattern language
│   └── websocket/    # WebSocket server
├── tests/            # Test suite
├── wavs/             # Sample library
├── SBTraxx/          # Track/pattern files
├── settings/         # Configuration files
├── build/            # Build output
└── cmake/            # CMake modules
```

## Current Branch

**destroyFX**: Working on audio effects (FX) system

## Important Notes

- The executable must be run from the project root directory to access the `wavs/` sample library
- Ableton Link provides network-synchronized tempo
- MIDI and audio devices are discovered at runtime
- WebSocket server allows remote control
- Pattern files stored in `SBTraxx/` directory
- Startup script: `startup.sb`

## Development Workflow

1. Code changes in `src/`
2. Format code: `cmake --build . --target format`
3. Build: `cmake --build .`
4. Run from root: `build/Sbsh`
5. Test with sample patterns in `SBTraxx/`

## Dependencies Management

- **Auto-downloaded** (via CMake FetchContent): Ableton Link, PerlinNoise
- **System packages**: PortAudio, PortMidi, libsndfile, readline, JsonCpp
- **Install script**: `install_deps.sh` handles most dependencies

## Testing

Test suite located in `tests/` directory.

## Additional Resources

- Demo video: https://www.youtube.com/watch?v=VRMtDkt9qRY
- Build instructions: `BUILD.md`
- Credits: `CREDITS.md`
