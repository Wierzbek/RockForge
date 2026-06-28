#pragma once
#include <fstream>
#include <string>
#include "audio_capture.h"

class ConfigManager {
public:
    static DetectionConfig LoadConfig(const std::string& filepath) {
        // Default fallback values
        DetectionConfig config = {0.001f, 0.2f, 15};
        
        std::ifstream file(filepath);
        if (!file.is_open()) return config;

        std::string key;
        float value;
        while (file >> key >> value) {
            if (key == "noise_gate_rms") config.noise_gate_rms = value;
            else if (key == "nn_confidence_threshold") config.nn_confidence_threshold = value;
            else if (key == "smoothing_window_frames") config.smoothing_window_frames = (int)value;
        }
        
        file.close();
        return config;
    }

    static void SaveConfig(const std::string& filepath, const DetectionConfig& config) {
        std::ofstream file(filepath);
        if (!file.is_open()) return;

        file << "noise_gate_rms " << config.noise_gate_rms << "\n";
        file << "nn_confidence_threshold " << config.nn_confidence_threshold << "\n";
        file << "smoothing_window_frames " << config.smoothing_window_frames << "\n";
        
        file.close();
    }
};
