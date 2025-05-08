// audio_manager.cpp
#include "audio_manager.h"
#include <sndfile.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <deque>

namespace AudioManager {

bool loadAudio(const std::string& filepath,
               std::vector<float>& buffer,
               int& sampleRate,
               int& channels) {
    SF_INFO sfinfo{};
    SNDFILE* sndfile = sf_open(filepath.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        std::cerr << "Error opening audio file: " << sf_strerror(nullptr) << std::endl;
        return false;
    }
    sampleRate = sfinfo.samplerate;
    channels = sfinfo.channels;
    sf_count_t frames = sfinfo.frames;
    buffer.resize(frames * channels);
    sf_count_t readcount = sf_readf_float(sndfile, buffer.data(), frames);
    sf_close(sndfile);
    return (readcount == frames);
}

bool saveAudio(const std::string& filepath,
               const std::vector<float>& buffer,
               int sampleRate,
               int channels) {
    SF_INFO sfinfo{};
    sfinfo.samplerate = sampleRate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE* sndfile = sf_open(filepath.c_str(), SFM_WRITE, &sfinfo);
    if (!sndfile) {
        std::cerr << "Error opening audio file for writing: " << sf_strerror(nullptr) << std::endl;
        return false;
    }
    sf_count_t frames = buffer.size() / channels;
    sf_writef_float(sndfile, buffer.data(), frames);
    sf_close(sndfile);
    return true;
}

void normalizeAudio(std::vector<float>& buffer) {
    float maxVal = 0.0f;
    for (float v : buffer) {
        maxVal = std::max(maxVal, std::abs(v));
    }
    if (maxVal > 0.0f) {
        float invMax = 1.0f / maxVal;
        for (float& v : buffer) v *= invMax;
    }
}

void applyFadeIn(std::vector<float>& buffer,
                 int sampleRate,
                 int channels,
                 int durationMs) {
    size_t totalSamples = buffer.size();
    size_t fadeSamples = (size_t)(durationMs / 1000.0f * sampleRate) * channels;
    for (size_t i = 0; i < fadeSamples && i < totalSamples; ++i) {
        float gain = (float)i / fadeSamples;
        buffer[i] *= gain;
    }
}

void applyFadeOut(std::vector<float>& buffer,
                  int sampleRate,
                  int channels,
                  int durationMs) {
    size_t totalSamples = buffer.size();
    size_t fadeSamples = (size_t)(durationMs / 1000.0f * sampleRate) * channels;
    for (size_t i = 0; i < fadeSamples && i < totalSamples; ++i) {
        float gain = (float)(fadeSamples - i) / fadeSamples;
        buffer[totalSamples - 1 - i] *= gain;
    }
}

void applyEqualizer(std::vector<float>& buffer,
                    int sampleRate,
                    int channels,
                    float lowCut,
                    float highCut) {
    // Due filtri RC a primo ordine: passa-alto e passa-basso
    float dt = 1.0f / sampleRate;
    float RC_low = 1.0f / (2 * M_PI * highCut);
    float alpha_low = dt / (RC_low + dt);
    float RC_high = 1.0f / (2 * M_PI * lowCut);
    float alpha_high = RC_high / (RC_high + dt);

    // process per canale
    for (int c = 0; c < channels; ++c) {
        float y_low = 0.0f;
        float y_high = buffer[c];
        for (size_t i = c; i < buffer.size(); i += channels) {
            float x = buffer[i];
            y_low += alpha_low * (x - y_low);
            y_high = alpha_high * (y_high + x - buffer[i - channels < 0 ? c : i - channels]);
            buffer[i] = y_low + y_high;
        }
    }
}

void applyNoiseReduction(std::vector<float>& buffer,
                          float threshold) {
    for (float& v : buffer) {
        if (std::abs(v) < threshold) v = 0.0f;
    }
}

void applyCompression(std::vector<float>& buffer,
                      float threshold,
                      float ratio) {
    for (float& v : buffer) {
        float mag = std::abs(v);
        if (mag > threshold) {
            float excess = mag - threshold;
            v = (v / mag) * (threshold + excess / ratio);
        }
    }
}

void applyReverb(std::vector<float>& buffer,
                 int sampleRate,
                 int channels,
                 float reverbTime) {
    // Feedback Delay Network semplice
    int delaySamples = (int)(0.03f * sampleRate) * channels; // 30ms
    float decay = std::exp(-3.0f / (reverbTime * sampleRate));
    std::deque<float> fb(delaySamples, 0.0f);
    for (size_t i = 0; i < buffer.size(); ++i) {
        float in = buffer[i];
        float out = in + fb.front() * decay;
        fb.pop_front();
        fb.push_back(out);
        buffer[i] = out;
    }
}

void applyDelay(std::vector<float>& buffer,
                int sampleRate,
                int channels,
                int delayMs,
                float feedback) {
    int delaySamples = (int)(delayMs / 1000.0f * sampleRate) * channels;
    std::deque<float> dbuf(delaySamples, 0.0f);
    for (size_t i = 0; i < buffer.size(); ++i) {
        float in = buffer[i];
        float delayed = dbuf.front();
        dbuf.pop_front();
        float out = in + delayed * feedback;
        dbuf.push_back(out);
        buffer[i] = out;
    }
}

void processAudio(std::vector<float>& buffer,
                  int sampleRate,
                  int channels) {
    normalizeAudio(buffer);
    applyFadeIn(buffer, sampleRate, channels, 100);
    applyFadeOut(buffer, sampleRate, channels, 100);
    applyEqualizer(buffer, sampleRate, channels, 200.0f, 5000.0f);
    applyNoiseReduction(buffer, 0.01f);
    applyCompression(buffer, 0.8f, 4.0f);
    applyReverb(buffer, sampleRate, channels, 1.5f);
    applyDelay(buffer, sampleRate, channels, 250, 0.5f);
}

void playAudio(const std::string& filepath) {
    std::string cmd = "aplay '" + filepath + "'";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Error playing audio file." << std::endl;
    }
}

} // namespace AudioManager
