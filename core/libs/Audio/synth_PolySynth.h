#pragma once

#include "AudioStream.h"

class Synth;

class PolySynth : public AudioStream
{
  public:
    PolySynth(void)
      : AudioStream(0, NULL)
    {
        for (float& gain : muteGain) // PNEM: Synth Mute: all parts play
            gain = 1.0f;
    }

    void init(Synth* trackerSynth);
    void setGain(int idx, float gainDB);

    // PNEM: Synth Mute. A muted part keeps rendering (voices, envelopes and LFOs run on); its
    // output and sends are silent, fading over one block when it changes.
    void setMuted(int idx, bool muted);
    bool isMuted(int idx) const;
    // Scales `count` buffers of a block from `from` to `target` (the last sample at target).
    static void applyMute(float* const* buffers, int count, float from, float target);
    virtual void update(void) override;

    static constexpr int kSend1Offset = 2 * kSynthParts;
    static constexpr int kSend2Offset = 3 * kSynthParts;
    static constexpr int kRightChannelOffset = kSynthParts;

  private:
    Synth* trackerSynth = nullptr;
    bool isBlocked = true;
    float gainDB[kSynthParts] = {};
    volatile bool muted[kSynthParts] = {};          // PNEM: Synth Mute, set from the UI
    float muteGain[kSynthParts];                    // PNEM: Synth Mute, the audio side's gain
};
