// PNEM CE: MIDI CC A-F as Performance columns (see perfMidiCc.h).

#include "perfMidiCc.h"

#include "Audio.h"
#include "config.h"
#include "mtMidi.h"
#include "mtSequencer.h"
#include "mtStructs.h"

namespace pnem::perfcc
{
namespace
{

constexpr uint8_t kChannels = 16;

// The value each column gives each track (0: none), and what each channel got last (0: nothing, or
// sent by something else since). Written by the main loop, read by the interrupt: single bytes.
volatile uint8_t trackValue[kPerformanceTracks][kCcs];
volatile uint8_t channelSent[kChannels][kCcs];

// The channel (1..16) of the MIDI instrument, or 0
AUDIO_CORE_RAM uint8_t midiChannel(uint8_t instrument)
{
    if (!isMidiInstrument(instrument))
    {
        return 0;
    }
    const int channel = instrumentIdxToMidiChannel(instrument);
    return channel >= 1 && channel <= kChannels ? (uint8_t)channel : 0;
}

AUDIO_CORE_RAM void send(uint8_t channel, uint8_t cc, uint8_t value)
{
    const uint8_t out = mtProject.values.midiInstrument[channel - 1].ccOut[cc];
    const uint8_t midiValue = value - 1;
    if (out < 128)
    {
        sendCC(out, midiValue, channel, false);
    }
    else if (out == MIDI_CC_SPECIAL_PC)
    {
        sendProgramChange(midiValue, channel, false);
    }
    else if (out == MIDI_CC_SPECIAL_CHAN_AT)
    {
        sendAfterTouch(midiValue, channel, false);
    }
    // Poly aftertouch needs a note; a column has none
    channelSent[channel - 1][cc] = value;
}

} // namespace

void set(uint8_t track, uint8_t cc, int16_t value)
{
    if (track >= kPerformanceTracks || cc >= kCcs || value < 1 || value > 128)
    {
        return;
    }
    trackValue[track][cc] = (uint8_t)value;

    const uint8_t channel = midiChannel(sequencer.pnemLastInstrument(track));
    if (channel == 0 || channelSent[channel - 1][cc] == value)
    {
        return;
    }
    // The interrupt sends notes and step FX on the same ports
    AudioNoInterrupts();
    send(channel, cc, (uint8_t)value);
    AudioInterrupts();
}

void clear(uint8_t track, uint8_t cc)
{
    if (track >= kPerformanceTracks || cc >= kCcs)
    {
        return;
    }
    trackValue[track][cc] = 0;
    const uint8_t channel = midiChannel(sequencer.pnemLastInstrument(track));
    if (channel != 0)
    {
        channelSent[channel - 1][cc] = 0;
    }
}

AUDIO_CORE_RAM void beforeMidiNote(uint8_t track, uint8_t instrument)
{
    if (track >= kPerformanceTracks)
    {
        return;
    }
    const uint8_t channel = midiChannel(instrument);
    if (channel == 0)
    {
        return;
    }
    for (uint8_t cc = 0; cc < kCcs; cc++)
    {
        const uint8_t value = trackValue[track][cc];
        if (value != 0 && channelSent[channel - 1][cc] != value)
        {
            send(channel, cc, value);
        }
    }
}

AUDIO_CORE_RAM void stepCcSent(uint8_t channel, uint8_t cc)
{
    if (channel >= 1 && channel <= kChannels && cc < kCcs)
    {
        channelSent[channel - 1][cc] = 0;
    }
}

} // namespace pnem::perfcc
