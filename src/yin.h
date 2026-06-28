#ifndef YIN_H
#define YIN_H

typedef struct {
    int buffer_size;
    int half_buffer_size;
    float threshold;
    float* yin_buffer;
} Yin;

void Yin_Init(Yin* yin, int buffer_size, float threshold);
void Yin_Free(Yin* yin);
float Yin_GetPitch(Yin* yin, const float* buffer, int sample_rate);

#endif // YIN_H
