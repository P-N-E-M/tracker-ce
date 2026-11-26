// PNEM CE: WT Cycle, a Sample Editor effect that makes a wavetable from any sample.
//
// Next to CE's WT Smoother, which stays as it is. It finds the pitch of the selection and spreads
// Window Num positions over the whole selection. For each position it measures the spectrum of single
// periods of the tone around that spot (averaged over the stretch of sample the position stands for,
// wider with Smooth), and builds one cycle of Window Len samples from it, with the same phase for each
// harmonic in every position. So every position loops seamlessly, and neighbouring positions differ only
// as much as the sound itself changes: moving from one to the next is barely audible. Meant for samples
// with a pitch.
//
// Window Num goes up to 1024, as long as Window Num x Window Len fits in the edit buffer's half
// (EDIT_BUFF_SIZE / 2 samples): at 2048, 960 positions.

#ifndef EFFECTOR_MTEFFECTWAVETABLECYCLE_H_
#define EFFECTOR_MTEFFECTWAVETABLECYCLE_H_

#include "mtEffect.h"

class mtEffectWavetableCycle : public mtEffect
{
  public:
    static constexpr int kLengthCount = 7; // Window Len 32, 64 .. 2048
    static constexpr int kMaxWindows = 1024;
    static constexpr int kMaxSmooth = 16;
    static constexpr uint32_t kMinPeriod = 16;   // 2756 Hz at 44.1 kHz
    static constexpr uint32_t kMaxPeriod = 2048; // 21.5 Hz
    static constexpr uint32_t kMaxWindowLen = 2048;
    // Work memory (floats): an FFT (re, im; YIN uses these), one channel's magnitudes, the FFT's
    // twiddle table, and per channel the phase (cos, sin) every window gets
    static constexpr uint32_t kWorkFloats =
      2 * kMaxWindowLen + kMaxWindowLen / 2 + kMaxWindowLen + kChannels * kMaxWindowLen;

    static uint32_t windowLength(int lengthIdx)
    {
        return 32u << lengthIdx;
    }
    // The most positions of this Window Len that fit in the edit buffer's half
    static int maxWindows(int lengthIdx)
    {
        const uint32_t fit = (EDIT_BUFF_SIZE / 2) / windowLength(lengthIdx);
        return fit < (uint32_t)kMaxWindows ? (int)fit : kMaxWindows;
    }

    virtual void setParameter(float& value, int idx) override;
    virtual bool startProcess() override;
    virtual int32_t processBlock() override;
    virtual bool getProcessState() override;
    virtual uint32_t getExpectedProcessLength() override;

  private:
    float mono(int32_t i) const;
    float read(int channel, float pos) const;
    float readSpan(int channel, float pos, float span) const;
    bool detectPeriod(float& period) const;
    float yinAt(uint32_t start, uint32_t integration, uint32_t maxPeriod) const;
    float localPeriod(float start, float near) const;
    bool findWork();
    void spectrumAt(int channel, float start, float period) const;
    void cycleWindow(float centre, float halfWidth, int16_t* const* dst);
    void finish();

    int windowsWanted = 256;
    int windows = 256; // windowsWanted, at most maxWindows(lengthIdx)
    int lengthIdx = kLengthCount - 1;
    int smooth = 2;

    // Per processing
    bool state = false;
    uint32_t windowLen = 0;
    uint32_t analysisLen = 0; // the FFT size a period is measured with (<= windowLen)
    uint32_t sourceLength = 0;
    float periodGlobal = 0.0f;
    float lastStart = 0.0f;   // the last position a period can start at
    float lastPeriod = 0.0f;  // the last spot's period: the next spot's search starts there
    float* work = nullptr;    // kWorkFloats, in the edit buffer behind the sample (see findWork)
    uint32_t windowIdx = 0;   // next window to make
};

extern mtEffectWavetableCycle sampleEditorWavetableCycle;

#endif /* EFFECTOR_MTEFFECTWAVETABLECYCLE_H_ */
