// PNEM CE: MIDI CC A-F as Performance columns.
//
// Six Performance FX types, MIDI CC A .. MIDI CC F. A column's top pad is "---" (nothing is sent),
// the three pads below it hold a value 0-127. A value goes to the MIDI instrument each affected track
// played last, as the CC that instrument's channel has for A-F (Instrument parameters, the same as
// the Send CC step FX): a CC number, or Program Change or channel aftertouch. A track that hasn't
// played a MIDI note yet gets it before its next one. Tracks on the same channel send it once.
// "---" sends nothing: the synth keeps the last value until its own knob moves.
//
// Values as the Performance engine keeps them: 0 is "---", 1..128 are CC values 0..127.

#ifndef PNEM_PERF_MIDI_CC_H_
#define PNEM_PERF_MIDI_CC_H_

#include <stdint.h>

namespace pnem::perfcc
{

constexpr uint8_t kCcs = 6; // A-F

// --- main loop (the Performance columns) ---

// A column's value reached a track (1..128).
void set(uint8_t track, uint8_t cc, int16_t value);

// A column went off on a track ("---", another type, Performance off).
void clear(uint8_t track, uint8_t cc);

// --- audio interrupt (the sequencer) ---

// Before a MIDI note on a track: values the instrument's channel doesn't have yet.
void beforeMidiNote(uint8_t track, uint8_t instrument);

// A Send CC step FX went out on a channel (1..16): the next note sends the column's value again.
void stepCcSent(uint8_t channel, uint8_t cc);

} // namespace pnem::perfcc

#endif /* PNEM_PERF_MIDI_CC_H_ */
