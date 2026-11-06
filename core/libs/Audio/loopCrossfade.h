// PNEM CE: Loop crossfade for Forward Loop and Backward Loop.
//
// A loop jumps at its seam: Forward Loop from the loop end back to the loop start; played
// backwards (Forward Loop reversed, or Backward Loop once it has turned) from the loop start to
// the loop end. With a crossfade of x frames, the last x frames before the seam are mixed, with
// equal-power gains, with the x frames that continue across it: the frames before the loop start,
// or after the loop end when the loop plays backwards. So the loop's end flows into its start.
// The length is limited to the loop and to the frames the sample has across the seam.
//
// The renders call this per sample while it plays in the zone; it runs in the audio interrupt.

#ifndef LIBS_AUDIO_LOOPCROSSFADE_H_
#define LIBS_AUDIO_LOOPCROSSFADE_H_

#include <stdint.h>

// sin(k / 512 * pi / 2) * 32767, k = 0..512: the incoming side's gain at k, the outgoing at 512 - k
extern const int16_t kLoopCrossfadeGain[513];

struct LoopCrossfadeZone
{
    int32_t edge = 0;     // the zone's frame farthest from the seam
    int32_t length = 0;   // frames in the zone, 0: no crossfade
    int32_t across = 0;   // incoming frame = playing frame + across
    uint32_t recip = 0;   // 2^31 / length
    bool towardsEnd = true; // the playhead moves up to the seam (else down to it)

    // A seam the playhead reaches moving up: at `seam` it jumps back down to `target`.
    // `before`: the frames the sample has below `target`.
    static LoopCrossfadeZone up(uint32_t frames, int32_t seam, int32_t target, int32_t before)
    {
        LoopCrossfadeZone z;
        const int32_t span = seam - target;
        const int32_t x = limit(frames, span, before);
        if (x >= 2)
        {
            z.edge = seam - x;
            z.length = x;
            z.across = -span;
            z.recip = 0x80000000u / (uint32_t)x;
            z.towardsEnd = true;
        }
        return z;
    }

    // A seam the playhead reaches moving down: at `seam` it jumps up to `target`.
    // `after`: the frames the sample has above `target` (less one, for interpolation).
    static LoopCrossfadeZone down(uint32_t frames, int32_t seam, int32_t target, int32_t after)
    {
        LoopCrossfadeZone z;
        const int32_t span = target - seam;
        const int32_t x = limit(frames, span, after);
        if (x >= 2)
        {
            z.edge = seam + x;
            z.length = x;
            z.across = span;
            z.recip = 0x80000000u / (uint32_t)x;
            z.towardsEnd = false;
        }
        return z;
    }

    // The playing frame's depth into the zone: 0 at its far edge, length - 1 at the seam; out of
    // the zone: length or more (unsigned).
    inline uint32_t depth(int32_t pos) const
    {
        return (uint32_t)(towardsEnd ? pos - edge : edge - pos);
    }

    inline bool covers(int32_t pos) const { return depth(pos) < (uint32_t)length; }

    // The outgoing sample mixed with the incoming one at depth d (covers() was true).
    inline int16_t mix(int16_t out, int16_t in, uint32_t d) const
    {
        // k = 512 (d + 1/2) / length, rounded: (2d + 1) * 2^31 / length < 2^32
        const uint32_t k = ((((2 * d + 1) * recip) >> 22) + 1u) >> 1;
        int32_t y = ((int32_t)kLoopCrossfadeGain[512 - k] * out + (int32_t)kLoopCrossfadeGain[k] * in +
                     16384) >>
                    15;
        if (y > 32767)
            y = 32767;
        else if (y < -32768)
            y = -32768;
        return (int16_t)y;
    }

  private:
    static int32_t limit(uint32_t frames, int32_t span, int32_t available)
    {
        int32_t x = frames > 0x7FFFFFFFu ? 0x7FFFFFFF : (int32_t)frames;
        if (x > span)
            x = span;
        if (x > available)
            x = available;
        return x;
    }
};

#endif /* LIBS_AUDIO_LOOPCROSSFADE_H_ */
