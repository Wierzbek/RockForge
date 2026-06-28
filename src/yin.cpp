#include "yin.h"
#include <stdlib.h>
#include <stdio.h>

void Yin_Init(Yin* yin, int buffer_size, float threshold) {
    yin->buffer_size = buffer_size;
    yin->half_buffer_size = buffer_size / 2;
    yin->threshold = threshold;
    yin->yin_buffer = (float*)malloc(sizeof(float) * yin->half_buffer_size);
}

void Yin_Free(Yin* yin) {
    if (yin->yin_buffer) {
        free(yin->yin_buffer);
        yin->yin_buffer = NULL;
    }
}

static void Yin_Difference(Yin* yin, const float* buffer) {
    int i, tau;
    float delta;
    for (tau = 0; tau < yin->half_buffer_size; tau++) {
        yin->yin_buffer[tau] = 0;
    }
    for (tau = 1; tau < yin->half_buffer_size; tau++) {
        for (i = 0; i < yin->half_buffer_size; i++) {
            delta = buffer[i] - buffer[i + tau];
            yin->yin_buffer[tau] += delta * delta;
        }
    }
}

static void Yin_CumulativeMeanNormalizedDifference(Yin* yin) {
    int tau;
    float running_sum = 0;
    yin->yin_buffer[0] = 1;
    for (tau = 1; tau < yin->half_buffer_size; tau++) {
        running_sum += yin->yin_buffer[tau];
        yin->yin_buffer[tau] *= tau / running_sum;
    }
}

static int Yin_AbsoluteThreshold(Yin* yin) {
    int tau;
    for (tau = 2; tau < yin->half_buffer_size; tau++) {
        if (yin->yin_buffer[tau] < yin->threshold) {
            while (tau + 1 < yin->half_buffer_size && yin->yin_buffer[tau + 1] < yin->yin_buffer[tau]) {
                tau++;
            }
            return tau;
        }
    }
    
    // Fallback: finding the global minimum
    float min_val = yin->yin_buffer[2];
    int min_tau = 2;
    for (tau = 3; tau < yin->half_buffer_size; tau++) {
        if (yin->yin_buffer[tau] < min_val) {
            min_val = yin->yin_buffer[tau];
            min_tau = tau;
        }
    }
    if (min_val < yin->threshold) return min_tau;
    return -1;
}

static float Yin_ParabolicInterpolation(Yin* yin, int tau_estimate) {
    float better_tau;
    int x0, x2;
    
    if (tau_estimate < 1) {
        x0 = tau_estimate;
    } else {
        x0 = tau_estimate - 1;
    }
    
    if (tau_estimate + 1 < yin->half_buffer_size) {
        x2 = tau_estimate + 1;
    } else {
        x2 = tau_estimate;
    }
    
    if (x0 == tau_estimate) {
        if (yin->yin_buffer[tau_estimate] <= yin->yin_buffer[x2]) {
            return tau_estimate;
        } else {
            return x2;
        }
    }
    if (x2 == tau_estimate) {
        if (yin->yin_buffer[tau_estimate] <= yin->yin_buffer[x0]) {
            return tau_estimate;
        } else {
            return x0;
        }
    }
    
    float s0, s1, s2;
    s0 = yin->yin_buffer[x0];
    s1 = yin->yin_buffer[tau_estimate];
    s2 = yin->yin_buffer[x2];
    
    better_tau = tau_estimate + (s2 - s0) / (2 * (2 * s1 - s2 - s0));
    return better_tau;
}

float Yin_GetPitch(Yin* yin, const float* buffer, int sample_rate) {
    Yin_Difference(yin, buffer);
    Yin_CumulativeMeanNormalizedDifference(yin);
    int tau_estimate = Yin_AbsoluteThreshold(yin);
    
    if (tau_estimate != -1) {
        float better_tau = Yin_ParabolicInterpolation(yin, tau_estimate);
        return (float)sample_rate / better_tau;
    }
    
    return -1;
}
