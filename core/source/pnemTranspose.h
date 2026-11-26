// PNEM CE: Transpose in scale (Project Settings).
//
// Project Settings > Transpose is Chromatic (CE) or In scale. In scale, turning the jog wheel or the
// encoder on notes in the Pattern screen moves each note a step of the project's Notes scale (from its
// Root note) instead of a semitone: one note to the next note of the scale, a selection of notes (a
// chord) to the next chord of the scale. A note outside the scale goes to the nearest one in that
// direction first. Octaves (Shift) stay octaves. With Notes scale on Chromatic it's CE's semitone.
//
// The setting is saved with the project, in two bytes of padding strMtValues already had
// (pnemTransposeTag): 'I', 'S' is In scale, anything else Chromatic. CE reads and writes the values
// whole, so a project keeps it, also through CE.

#ifndef PNEM_TRANSPOSE_H_
#define PNEM_TRANSPOSE_H_

#include <stdint.h>

namespace pnem::transpose
{

// The menu's value: 0 Chromatic, 1 In scale
extern uint8_t menuValue;

// After a project's values are read: the menu from the project
void fromProject();

// The menu changed: into the project
void menuChanged();

// Whether the jog moves notes in scale steps: In scale, and a Notes scale other than Chromatic
bool inScale();

// The note (0..STEP_NOTE_MAX) moved `steps` steps of the project's scale; it stays where it is when
// the scale has no further note in that direction.
int16_t step(int16_t note, int16_t steps);

} // namespace pnem::transpose

#endif /* PNEM_TRANSPOSE_H_ */
