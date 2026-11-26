// PNEM CE: WT Cycle (see mtEffectWavetableCycle.h).

#include "mtEffectWavetableCycle.h"
#include "pnemFft.h"

#include <algorithm>
#include <math.h>

mtEffectWavetableCycle sampleEditorWavetableCycle;

namespace
{
constexpr float kDipThreshold = 0.15f; // YIN: the first dip below this is the period
constexpr uint32_t kIntegration = 1024;
constexpr int kDetectPoints = 5;
constexpr uint32_t kBlock = 8192;
constexpr uint32_t kBlockWindows = 8;  // at most this many windows per processBlock
constexpr float kLocalMin = 0.8f;      // a spot's period lies within this range of the sample's period
constexpr float kLocalMax = 1.25f;
constexpr uint32_t kLocalPoints = 64;  // points compared per lag when finding a spot's period
constexpr float kLocalNear = 0.05f;    // first searched within this much of the last spot's period
constexpr uint32_t kMaxSpanPoints = 64;
constexpr uint32_t kMaxAnalyses = 8;   // periods measured per window

// The work memory's parts (offsets in floats)
constexpr uint32_t kWorkIm = mtEffectWavetableCycle::kMaxWindowLen;
constexpr uint32_t kWorkMagnitude = 2 * mtEffectWavetableCycle::kMaxWindowLen;
constexpr uint32_t kWorkTable = kWorkMagnitude + mtEffectWavetableCycle::kMaxWindowLen / 2;
constexpr uint32_t kWorkPhase = kWorkTable + mtEffectWavetableCycle::kMaxWindowLen;
static_assert(kWorkPhase + mtEffect::kChannels * mtEffectWavetableCycle::kMaxWindowLen ==
                mtEffectWavetableCycle::kWorkFloats,
              "the work memory's parts");
static_assert(mtEffectWavetableCycle::kMaxPeriod + 2 <= kWorkMagnitude,
              "YIN's buffer fits in the FFT's part of the work memory");

int16_t saturate(float v)
{
    const int32_t i = (int32_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
    return (int16_t)std::max<int32_t>(-32768, std::min<int32_t>(32767, i));
}

} // namespace

void mtEffectWavetableCycle::setParameter(float& value, int idx)
{
    const int v = (int)(value + 0.5f);
    switch (idx)
    {
        case 0: windowsWanted = std::max(1, std::min(kMaxWindows, v)); break;
        case 1: lengthIdx = std::max(0, std::min(kLengthCount - 1, v)); break;
        case 2: smooth = std::max(0, std::min(kMaxSmooth, v)); break;
        default: assert(false); break;
    }
    windows = std::min(windowsWanted, maxWindows(lengthIdx));
}

uint32_t mtEffectWavetableCycle::getExpectedProcessLength()
{
    return (uint32_t)windows * windowLength(lengthIdx);
}

bool mtEffectWavetableCycle::getProcessState()
{
    return state;
}

float mtEffectWavetableCycle::mono(int32_t i) const
{
    i = std::max<int32_t>(0, std::min<int32_t>((int32_t)sourceLength - 1, i));
    float sum = 0.0f;
    for (int channel = 0; channel < kChannels; channel++)
    {
        sum += confirmed[channel].selection.addr[i];
    }
    return sum * (1.0f / kChannels);
}

// The sample at a fractional position (4-point Hermite)
float mtEffectWavetableCycle::read(int channel, float pos) const
{
    const int16_t* src = confirmed[channel].selection.addr;
    const int32_t last = (int32_t)sourceLength - 1;
    int32_t i = (int32_t)pos;
    if ((float)i > pos)
    {
        i--; // floor for negative positions
    }
    const float f = pos - (float)i;
    float xm1, x0, x1, x2;
    if (i >= 1 && i + 2 <= last)
    {
        const int16_t* p = src + i;
        xm1 = p[-1], x0 = p[0], x1 = p[1], x2 = p[2];
    }
    else
    {
        auto at = [&](int32_t k) { return (float)src[std::max<int32_t>(0, std::min<int32_t>(last, k))]; };
        xm1 = at(i - 1), x0 = at(i), x1 = at(i + 1), x2 = at(i + 2);
    }
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

// The sample at a position, averaged over `span` samples when a window has fewer samples than the
// cycle (so it doesn't alias)
float mtEffectWavetableCycle::readSpan(int channel, float pos, float span) const
{
    if (span <= 1.0f)
    {
        return read(channel, pos);
    }
    const uint32_t points = std::min<uint32_t>(kMaxSpanPoints, (uint32_t)ceilf(span));
    const float step = span / points;
    float sum = 0.0f;
    float p = pos - 0.5f * span + 0.5f * step;
    for (uint32_t i = 0; i < points; i++, p += step)
    {
        sum += read(channel, p);
    }
    return sum / points;
}

// YIN at one point of the sample: the period (fractional)
float mtEffectWavetableCycle::yinAt(uint32_t start, uint32_t integration, uint32_t maxPeriod) const
{
    // Both channels summed, straight from the buffers (all indices are inside the sample): the inner
    // loop runs up to integration x maxPeriod times per point
    const int16_t* left = confirmed[0].selection.addr + start;
    const int16_t* right = confirmed[kChannels - 1].selection.addr + start;
    float* cm = work;
    cm[0] = 1.0f;
    float sum = 0.0f;
    bool below = false;
    uint32_t dip = 0;
    for (uint32_t t = 1; t <= maxPeriod + 1; t++)
    {
        float d = 0.0f;
        for (uint32_t j = 0; j < integration; j++)
        {
            const float diff = (float)((left[j] + right[j]) - (left[j + t] + right[j + t]));
            d += diff * diff;
        }
        sum += d;
        cm[t] = sum > 0.0f ? d * t / sum : 1.0f;
        if (t > kMinPeriod && t - 1 <= maxPeriod)
        {
            if (!below && cm[t - 1] < kDipThreshold)
            {
                below = true;
            }
            if (below && cm[t] >= cm[t - 1])
            {
                dip = t - 1; // the bottom of the first dip below the threshold
                break;
            }
        }
    }
    if (dip == 0)
    {
        // No dip below the threshold: the lowest point
        dip = kMinPeriod;
        for (uint32_t t = kMinPeriod + 1; t <= maxPeriod; t++)
        {
            if (cm[t] < cm[dip])
            {
                dip = t;
            }
        }
    }
    const float a = cm[dip - 1], b = cm[dip], c = cm[dip + 1];
    const float den = a - 2.0f * b + c;
    const float offset = den > 0.0f ? 0.5f * (a - c) / den : 0.0f;
    return (float)dip + std::max(-0.5f, std::min(0.5f, offset));
}

// The sample's period: the median of a few points spread over it
bool mtEffectWavetableCycle::detectPeriod(float& period) const
{
    const uint32_t length = sourceLength;
    const uint32_t integration = std::min<uint32_t>(kIntegration, length / 3);
    const uint32_t maxPeriod =
      std::min<uint32_t>(kMaxPeriod, std::min<uint32_t>(length / 2, length - integration - 3));
    if (integration < kMinPeriod || maxPeriod < kMinPeriod + 2)
    {
        return false;
    }
    const uint32_t room = length - integration - maxPeriod - 3;

    float periods[kDetectPoints];
    for (int q = 0; q < kDetectPoints; q++)
    {
        const uint32_t start = (uint32_t)((float)room * (q + 0.5f) / kDetectPoints);
        periods[q] = yinAt(start, integration, maxPeriod);
    }
    std::sort(periods, periods + kDetectPoints);
    period = periods[kDetectPoints / 2];
    return true;
}

// The period of the tone at `start`: the lag (within kLocalMin..kLocalMax of the sample's period) at
// which the sample repeats best, fractional. The search starts within kLocalNear of `near` (the last
// spot's period) and only takes the whole range when the best lag lies at the edge of that.
// `start` is at most lastStart, so every index below is inside the sample.
float mtEffectWavetableCycle::localPeriod(float start, float near) const
{
    const int32_t lowest = std::max<int32_t>(kMinPeriod, (int32_t)floorf(periodGlobal * kLocalMin));
    const int32_t highest = std::min<int32_t>(kMaxPeriod, (int32_t)ceilf(periodGlobal * kLocalMax));
    const int32_t n = (int32_t)periodGlobal;
    const int32_t step = std::max<int32_t>(1, n / (int32_t)kLocalPoints);
    const int32_t s = std::max<int32_t>(0, (int32_t)start);
    const int16_t* left = confirmed[0].selection.addr + s;
    const int16_t* right = confirmed[kChannels - 1].selection.addr + s;
    auto difference = [&](int32_t t) {
        float d = 0.0f;
        for (int32_t j = 0; j < n; j += step)
        {
            const float diff = (float)((left[j] + right[j]) - (left[j + t] + right[j + t]));
            d += diff * diff;
        }
        return d;
    };
    auto search = [&](int32_t lo, int32_t hi, float& bestD) {
        int32_t best = lo;
        bestD = difference(lo);
        for (int32_t t = lo + 1; t <= hi; t++)
        {
            const float d = difference(t);
            if (d < bestD)
            {
                bestD = d;
                best = t;
            }
        }
        return best;
    };
    float bestD;
    const int32_t lo = std::max(lowest, (int32_t)floorf(near * (1.0f - kLocalNear)));
    const int32_t hi = std::min(highest, (int32_t)ceilf(near * (1.0f + kLocalNear)));
    int32_t best = lo < hi ? search(lo, hi, bestD) : lowest;
    if (lo >= hi || (best == lo && lo > lowest) || (best == hi && hi < highest))
    {
        best = search(lowest, highest, bestD);
    }
    if (best <= lowest || best >= highest)
    {
        return periodGlobal; // no dip inside the range: keep the sample's period
    }
    const float a = difference(best - 1), c = difference(best + 1);
    const float den = a - 2.0f * bestD + c;
    const float offset = den > 0.0f ? 0.5f * (a - c) / den : 0.0f;
    return (float)best + std::max(-0.5f, std::min(0.5f, offset));
}

// The work memory: CE has no SDRAM to spare, but the half of the edit buffer the sample is in has
// room behind the sample (EDIT_BUFF_SIZE / 2 samples each; apply swaps the halves). Nothing reads
// it; the effect only fails when the sample nearly fills its half.
bool mtEffectWavetableCycle::findWork()
{
    const int16_t* area = confirmed[kChannels - 1].area.addr;
    const uintptr_t from = ((uintptr_t)(area + confirmed[kChannels - 1].area.length) + 3) & ~(uintptr_t)3;
    const uintptr_t end = (uintptr_t)(area + EDIT_BUFF_SIZE / 2);
    if (from + kWorkFloats * sizeof(float) > end)
    {
        work = nullptr;
        return false;
    }
    work = (float*)from;
    return true;
}

// The spectrum of one period from `start` (scaled to analysisLen samples) in the work memory's FFT
// (bins 0..analysisLen / 2).
// The small difference between its start and its end is spread over it first, so the period loops.
void mtEffectWavetableCycle::spectrumAt(int channel, float start, float period) const
{
    float* re = work;
    const float ratio = period / analysisLen;
    const float seam = readSpan(channel, start + period, ratio) - readSpan(channel, start, ratio);
    for (uint32_t j = 0; j < analysisLen; j++)
    {
        re[j] = readSpan(channel, start + j * ratio, ratio) - seam * j / analysisLen;
    }
    pnem::fft::realFft(re, work + kWorkIm, analysisLen, work + kWorkTable, windowLen);
}

// One window for each channel: the magnitudes of up to kMaxAnalyses periods between centre -
// halfWidth and centre + halfWidth (weighted towards the centre), with the reference phases, as one
// cycle of windowLen samples
void mtEffectWavetableCycle::cycleWindow(float centre, float halfWidth, int16_t* const* dst)
{
    float* re = work;
    float* im = work + kWorkIm;
    float* magnitude = work + kWorkMagnitude;
    const uint32_t bins = analysisLen / 2;

    float starts[kMaxAnalyses], periods[kMaxAnalyses], weights[kMaxAnalyses];
    const uint32_t count = std::max<uint32_t>(
      1, std::min<uint32_t>(kMaxAnalyses, (uint32_t)(2.0f * halfWidth / periodGlobal + 1.5f)));
    float weightSum = 0.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float offset = count > 1 ? -halfWidth + 2.0f * halfWidth * i / (count - 1) : 0.0f;
        weights[i] = 1.0f - fabsf(offset) / (halfWidth + periodGlobal);
        starts[i] = std::max(0.0f, std::min(lastStart, centre + offset));
        periods[i] = lastPeriod = localPeriod(starts[i], lastPeriod);
        weightSum += weights[i];
    }
    // An FFT of analysisLen gives a harmonic of amplitude a a magnitude of a * analysisLen / 2; the
    // inverse of windowLen needs a * windowLen / 2
    const float scale = (float)windowLen / (weightSum * analysisLen);

    for (int channel = 0; channel < kChannels; channel++)
    {
        for (uint32_t h = 0; h < bins; h++)
        {
            magnitude[h] = 0.0f;
        }
        for (uint32_t i = 0; i < count; i++)
        {
            spectrumAt(channel, starts[i], periods[i]);
            for (uint32_t h = 1; h < bins; h++)
            {
                magnitude[h] += weights[i] * sqrtf(re[h] * re[h] + im[h] * im[h]);
            }
        }
        // These magnitudes with the reference phases; nothing above the measured harmonics
        const float* phase = work + kWorkPhase + channel * kMaxWindowLen;
        re[0] = im[0] = 0.0f;
        for (uint32_t h = 1; h < bins; h++)
        {
            const float m = magnitude[h] * scale;
            re[h] = m * phase[2 * h];
            im[h] = m * phase[2 * h + 1];
        }
        for (uint32_t h = bins; h <= windowLen / 2; h++)
        {
            re[h] = im[h] = 0.0f;
        }
        pnem::fft::inverseRealFft(re, im, windowLen, work + kWorkTable, windowLen);
        for (uint32_t j = 0; j < windowLen; j++)
        {
            dst[channel][j] = saturate(re[j]);
        }
    }
}

bool mtEffectWavetableCycle::startProcess()
{
    state = false;
    windowLen = windowLength(lengthIdx);
    sourceLength = confirmed[0].selection.length;
    windowIdx = 0;

    if (!findWork())
    {
        return false;
    }
    // The sample's period (without a clear pitch, the best guess: the effect is meant for samples
    // with one)
    if (!detectPeriod(periodGlobal))
    {
        return false;
    }
    // A period starting at lastStart, at its longest, still has room for the comparison and
    // the interpolation
    lastStart = (float)sourceLength - 2.0f * ceilf(periodGlobal * kLocalMax) - 4.0f;
    if (lastStart < 0.0f)
    {
        return false;
    }
    // Measure with the smallest power of 2 a period fits in: a period has no harmonics above
    // half its length, so the rest of a long window would stay empty anyway
    analysisLen = 32;
    while (analysisLen < windowLen && analysisLen < periodGlobal * kLocalMax)
    {
        analysisLen <<= 1;
    }
    pnem::fft::makeTable(work + kWorkTable, windowLen);

    // Every window gets the phases of the period in the middle of the selection
    const float refStart = 0.5f * lastStart;
    const float refPeriod = localPeriod(refStart, periodGlobal);
    lastPeriod = periodGlobal;
    const float* re = work;
    const float* im = work + kWorkIm;
    for (int channel = 0; channel < kChannels; channel++)
    {
        spectrumAt(channel, refStart, refPeriod);
        float* phase = work + kWorkPhase + channel * kMaxWindowLen;
        for (uint32_t h = 0; h < analysisLen / 2; h++)
        {
            const float r = sqrtf(re[h] * re[h] + im[h] * im[h]);
            phase[2 * h] = r > 0.0f ? re[h] / r : 1.0f;
            phase[2 * h + 1] = r > 0.0f ? im[h] / r : 0.0f;
        }
    }
    state = true;
    return true;
}

int32_t mtEffectWavetableCycle::processBlock()
{
    if (!state)
    {
        return -1;
    }
    uint32_t written = 0;
    uint32_t made = 0;
    while (windowIdx < (uint32_t)windows && written < kBlock && made < kBlockWindows)
    {
        int16_t* dst[kChannels];
        for (int channel = 0; channel < kChannels; channel++)
        {
            dst[channel] = processed[channel].selection.addr + windowIdx * windowLen;
        }
        // Each window stands for an equal stretch of the selection; Smooth widens the stretch its
        // spectrum is averaged over by as many windows on each side
        const float stretch = windows > 1 ? lastStart / (windows - 1) : lastStart;
        const float centre = windows > 1 ? stretch * windowIdx : 0.5f * lastStart;
        const float halfWidth = 0.5f * stretch * (1 + 2 * smooth);
        cycleWindow(centre, halfWidth, dst);
        windowIdx++;
        made++;
        written += windowLen;
    }
    if (windowIdx >= (uint32_t)windows)
    {
        finish();
    }
    return written;
}

void mtEffectWavetableCycle::finish()
{
    state = false;
    for (int channel = 0; channel < kChannels; channel++)
    {
        processed[channel].selection.length = windows * windowLen;
        processed[channel].area.length =
          confirmed[channel].area.length +
          (processed[channel].selection.length - confirmed[channel].selection.length);
    }
}
