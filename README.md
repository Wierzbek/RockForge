# RockForge

RockForge is an advanced, real-time polyphonic guitar tracking and practice suite built in C++ and Raylib.

## Features

- **Real-time Polyphonic Pitch Tracking**: Powered by an integration of Spotify's Basic Pitch (ONNX Neural Network), RockForge captures your dry guitar DI signal and instantly transcribes polyphonic chords and notes.
- **Smart Hand Positioning**: Employs a custom recursive backtracking algorithm to automatically assign transcribed pitches to the most ergonomic fretboard positions, minimizing hand stretch and rendering realistic tablature (e.g. solving the problem of high notes collapsing to open strings).
- **Dynamic Harmonic Filtering**: Automatically trims ghost notes and overtones by analyzing the fundamental frequency's confidence and applying a dynamic 40% threshold gate.
- **Interactive Practice Mode**: Turn your PC into a guitar trainer. The app generates scrolling multi-line tablature sheets and features an interactive grading system that turns notes green when played correctly in time with the playhead. 
- **Real-Time Calibration**: A built-in UI for tuning the RMS Noise Gate, Neural Net Confidence, and Temporal Smoothing on the fly, with settings persisted automatically.

## Requirements

- Windows 10/11
- A guitar and an audio interface (DI signal)
- Minimum 8GB RAM (ONNX inference relies on steady CPU execution)

## Setup & Build

This project relies on the following libraries:
- `raylib` (5.0+) for the user interface and rendering.
- `onnxruntime` (1.20.1) for executing the polyphonic model.
- `miniaudio` (integrated via raylib) for raw WASAPI device capture.
- `basicpitch.cpp` for parsing ONNX outputs into discrete MIDI events.

To build the project:
1. Ensure you have the `mingw64` toolchain installed and accessible.
2. Run `.\build.bat` in the root directory.
3. The executable `rockforge.exe` will be generated in the root directory.

## Usage

1. Launch `rockforge.exe`.
2. Connect your guitar to your audio interface.
3. Press the number corresponding to your audio input device.
4. **Free Play Mode**: Strum a chord and watch the tablature instantly recognize and render your voicing.
5. **Practice Mode**: Load an exercise, hit PLAY, and wait for the 4-beat count-in. Play along with the scrolling tab to get scored dynamically.

## License
MIT License.
