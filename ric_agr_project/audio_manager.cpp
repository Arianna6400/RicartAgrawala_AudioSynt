// audio_manager.cpp
#include "audio_manager.h"
#include <sndfile.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <deque>
#include <filesystem>

namespace fs = std::filesystem;

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

// Funzione per URL-encoding minimale
std::string url_encode(const std::string& str) {
    std::ostringstream encoded;
    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
        }
    }
    return encoded.str();
}

// Funzione chiave: genera l’audio da testo
bool synthesizeTextToAudio(const std::string& text,
    std::vector<float>& buffer,
    int& sampleRate,
    int& channels,
    int node_id) {
std::string output_file = "output_audio/output_" + std::to_string(node_id) + ".wav";
std::string safe_text = url_encode(text);
std::string cmd = "python3 audio_synthesizer/synthesizer.py " + std::to_string(node_id) + " \"" + safe_text + "\"";

int result = std::system(cmd.c_str());
if (result != 0) {
std::cerr << "synthesizer.py execution failed\n";
return false;
}

if (!fs::exists(output_file)) {
std::cerr << "Generated audio file not found: " << output_file << "\n";
return false;
}

return loadAudio(output_file, buffer, sampleRate, channels);
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

void processAudio(std::vector<float>& buffer,
                  int sampleRate,
                  int channels) {
    normalizeAudio(buffer);
}

void playAudio(const std::string& filepath) {
    std::string cmd = "vlc --intf dummy --no-video --no-dbus --play-and-exit \"" + filepath + "\"";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Error playing audio file." << std::endl;
    }
}

// FUNZIONI OPZIONALI PER APPLICARE EFFETTI SONORI (NON USATE)
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

} // namespace AudioManager
