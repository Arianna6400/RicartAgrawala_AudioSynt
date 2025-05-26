// audio_manager.h
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <vector>
#include <string>

namespace AudioManager {

    bool loadAudio(const std::string& filepath,
                   std::vector<float>& buffer,
                   int& sampleRate,
                   int& channels);

    bool saveAudio(const std::string& filepath,
                   const std::vector<float>& buffer,
                   int sampleRate,
                   int channels);

    bool synthesizeTextToAudio(const std::string& text,
                    std::vector<float>& buffer,
                    int& sampleRate,
                    int& channels,
                    int node_id);

    /**
     * Normalizza il buffer audio al picco massimo.
     */
    void normalizeAudio(std::vector<float>& buffer);

    /**
     * Applica un fade-in sul buffer audio.
     * @param durationMs Durata del fade-in in millisecondi.
     */
    void applyFadeIn(std::vector<float>& buffer,
                     int sampleRate,
                     int channels,
                     int durationMs);

    /**
     * Applica un fade-out sul buffer audio.
     * @param durationMs Durata del fade-out in millisecondi.
     */
    void applyFadeOut(std::vector<float>& buffer,
                      int sampleRate,
                      int channels,
                      int durationMs);

    /**
     * Equalizzatore passa-basso e passa-alto semplice a primo ordine.
     * @param lowCut Frequenza di taglio basso (Hz).
     * @param highCut Frequenza di taglio alto (Hz).
     */
    void applyEqualizer(std::vector<float>& buffer,
                        int sampleRate,
                        int channels,
                        float lowCut,
                        float highCut);

    /**
     * Noise gate: azzera i campioni al di sotto di una soglia.
     * @param threshold Soglia di ampiezza.
     */
    void applyNoiseReduction(std::vector<float>& buffer,
                              float threshold);

    /**
     * Compressione dinamica semplice.
     * @param threshold Soglia in ampiezza (0.0-1.0).
     * @param ratio Rapporto di compressione (>1.0).
     */
    void applyCompression(std::vector<float>& buffer,
                          float threshold,
                          float ratio);

    /**
     * Riverbero semplice basato su feedback delay network.
     */
    void applyReverb(std::vector<float>& buffer,
                     int sampleRate,
                     int channels,
                     float reverbTime);

    /**
     * Delay semplice con buffer circolare.
     * @param delayMs Tempo di delay in millisecondi.
     * @param feedback Quantità di feedback (0.0-1.0).
     */
    void applyDelay(std::vector<float>& buffer,
                    int sampleRate,
                    int channels,
                    int delayMs,
                    float feedback);

    /**
     * Esegue la processazione completa: normalizzazione e tutti gli effetti.
     */
    void processAudio(std::vector<float>& buffer,
                      int sampleRate,
                      int channels);

    /**
     * Riproduce un file WAV esterno via comando di sistema.
     */
    void playAudio(const std::string& filepath);

} // namespace AudioManager

#endif // AUDIO_MANAGER_H