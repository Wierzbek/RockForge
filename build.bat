@echo off
set GPP=mingw64\bin\g++.exe

echo Compiling RockForge C++ with ONNX...
%GPP% src\main.cpp src\audio_capture.cpp src\note_math.cpp src\yin.cpp src\song_data.cpp src\tab_renderer.cpp src\fretboard_view.cpp src\file_manager.cpp ^
basicpitch.cpp\src\ort_inference.cpp basicpitch.cpp\src\midi_notes.cpp ^
-Iinclude -Ibasicpitch.cpp\src -Ibasicpitch.cpp\vendor\eigen -Ibasicpitch.cpp\vendor\libremidi\include -Ibasicpitch.cpp\vendor\onnxruntime\include ^
-Llib -Lonnxruntime-win-x64-1.20.1\lib ^
-lraylib -lgdi32 -lwinmm -lonnxruntime -static-libgcc -static-libstdc++ ^
-std=c++20 -D_stdcall=__stdcall -DLIBREMIDI_HEADER_ONLY=1 -O3 -o rockforge.exe

if %errorlevel% neq 0 (
    echo Build failed!
) else (
    echo Build successful!
    copy /Y onnxruntime-win-x64-1.20.1\lib\onnxruntime.dll . > nul
)
