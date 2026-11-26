// PNEM CE: a small FFT for the Sample Editor's WT Cycle (no CMSIS-DSP on every platform).
//
// Radix-2, in place, with a twiddle table of the largest size used. realFft and inverseRealFft give
// and take the half spectrum X[0..n/2] like numpy's rfft and irfft (irfft scales by 1/n).

#ifndef EFFECTOR_PNEMFFT_H_
#define EFFECTOR_PNEMFFT_H_

#include <math.h>
#include <stdint.h>

namespace pnem::fft
{

// tw[2k], tw[2k + 1] = cos, sin of -2 pi k / size, for k < size / 2
inline void makeTable(float* tw, uint32_t size)
{
    for (uint32_t k = 0; k < size / 2; k++)
    {
        const float angle = -6.28318530718f * k / size;
        tw[2 * k] = cosf(angle);
        tw[2 * k + 1] = sinf(angle);
    }
}

// Complex FFT of n (a power of 2, <= size) values; the inverse isn't scaled
inline void complexFft(float* re, float* im, uint32_t n, bool inverse, const float* tw, uint32_t size)
{
    for (uint32_t i = 1, j = 0; i < n; i++)
    {
        uint32_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
        {
            j ^= bit;
        }
        j ^= bit;
        if (i < j)
        {
            const float r = re[i], m = im[i];
            re[i] = re[j];
            im[i] = im[j];
            re[j] = r;
            im[j] = m;
        }
    }
    const float sign = inverse ? -1.0f : 1.0f;
    for (uint32_t len = 2; len <= n; len <<= 1)
    {
        const uint32_t half = len >> 1;
        const uint32_t stride = 2 * (size / len);
        for (uint32_t i = 0; i < n; i += len)
        {
            const float* w = tw;
            for (uint32_t j = 0; j < half; j++, w += stride)
            {
                const float wRe = w[0], wIm = sign * w[1];
                const uint32_t u = i + j, v = u + half;
                const float tRe = re[v] * wRe - im[v] * wIm;
                const float tIm = re[v] * wIm + im[v] * wRe;
                re[v] = re[u] - tRe;
                im[v] = im[u] - tIm;
                re[u] += tRe;
                im[u] += tIm;
            }
        }
    }
}

// The spectrum of n real values re[0..n) (n >= 4, <= size): X[0..n/2] in re, im (im needs n/2 + 1)
inline void realFft(float* re, float* im, uint32_t n, const float* tw, uint32_t size)
{
    const uint32_t h = n / 2;
    for (uint32_t m = 0; m < h; m++)
    {
        im[m] = re[2 * m + 1];
    }
    for (uint32_t m = 1; m < h; m++)
    {
        re[m] = re[2 * m];
    }
    complexFft(re, im, h, false, tw, size);

    const float z0Re = re[0], z0Im = im[0];
    re[0] = z0Re + z0Im;
    im[0] = 0.0f;
    re[h] = z0Re - z0Im;
    im[h] = 0.0f;
    const uint32_t stride = 2 * (size / n);
    for (uint32_t k = 1; k <= h / 2; k++)
    {
        const uint32_t j = h - k;
        const float kRe = re[k], kIm = im[k], jRe = re[j], jIm = im[j];
        const float eRe = 0.5f * (kRe + jRe), eIm = 0.5f * (kIm - jIm); // the even samples' part
        const float oRe = 0.5f * (kIm + jIm), oIm = -0.5f * (kRe - jRe); // the odd samples' part
        const float wRe = tw[k * stride], wIm = tw[k * stride + 1];
        const float tRe = wRe * oRe - wIm * oIm, tIm = wRe * oIm + wIm * oRe;
        re[k] = eRe + tRe;
        im[k] = eIm + tIm;
        re[j] = eRe - tRe;
        im[j] = -eIm + tIm;
    }
}

// n real values in re[0..n) from the half spectrum X[0..n/2] in re, im; scaled by 1 / n
inline void inverseRealFft(float* re, float* im, uint32_t n, const float* tw, uint32_t size)
{
    const uint32_t h = n / 2;
    const float x0 = re[0], xh = re[h];
    const uint32_t stride = 2 * (size / n);
    for (uint32_t k = 1; k <= h / 2; k++)
    {
        const uint32_t j = h - k;
        const float kRe = re[k], kIm = im[k], jRe = re[j], jIm = im[j];
        // even part E = (X[k] + conj X[j]) / 2, odd part O = conj(W^k) (X[k] - conj X[j]) / 2
        const float eRe = 0.5f * (kRe + jRe), eIm = 0.5f * (kIm - jIm);
        const float dRe = 0.5f * (kRe - jRe), dIm = 0.5f * (kIm + jIm);
        const float wRe = tw[k * stride], wIm = -tw[k * stride + 1];
        const float oRe = wRe * dRe - wIm * dIm, oIm = wRe * dIm + wIm * dRe;
        // Z[k] = E + i O; Z[j] = conj E + i conj O
        re[k] = eRe - oIm;
        im[k] = eIm + oRe;
        re[j] = eRe + oIm;
        im[j] = -eIm + oRe;
    }
    re[0] = 0.5f * (x0 + xh);
    im[0] = 0.5f * (x0 - xh);
    complexFft(re, im, h, true, tw, size);
    const float scale = 1.0f / h;
    for (int32_t m = (int32_t)h - 1; m >= 0; m--)
    {
        const float odd = im[m] * scale;
        re[2 * m] = re[m] * scale;
        re[2 * m + 1] = odd;
    }
}

} // namespace pnem::fft

#endif /* EFFECTOR_PNEMFFT_H_ */
