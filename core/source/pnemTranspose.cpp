// PNEM CE: Transpose in scale (see pnemTranspose.h).

#include "pnemTranspose.h"

#include "fileManager.h"
#include "mtSequencer.h"
#include "mtStructs.h"
#include "scales.h"

namespace pnem::transpose
{
namespace
{
constexpr uint8_t kTag0 = 'I';
constexpr uint8_t kTag1 = 'S';

bool tagged()
{
    return mtProject.values.pnemTransposeTag[0] == kTag0 && mtProject.values.pnemTransposeTag[1] == kTag1;
}
} // namespace

uint8_t menuValue = 0;

void fromProject()
{
    menuValue = tagged() ? 1 : 0;
}

void menuChanged()
{
    mtProject.values.pnemTransposeTag[0] = menuValue ? kTag0 : 0;
    mtProject.values.pnemTransposeTag[1] = menuValue ? kTag1 : 0;
    newFileManager.setProjectStructChanged();
}

bool inScale()
{
    return tagged() && mtProject.values.padBoardScale != 0 && mtProject.values.padBoardScale <= MAX_SCALE;
}

int16_t step(int16_t note, int16_t steps)
{
    const uint8_t root = mtProject.values.padBoardRootNote;
    const uint8_t scale = mtProject.values.padBoardScale;
    const int16_t dir = steps > 0 ? 1 : -1;
    int16_t at = note;
    for (int16_t i = 0; i != steps; i += dir)
    {
        int16_t next = at + dir;
        while (next >= 0 && next <= Sequencer::STEP_NOTE_MAX && !isInScale((uint8_t)next, root, scale))
        {
            next += dir;
        }
        if (next < 0 || next > Sequencer::STEP_NOTE_MAX)
        {
            break; // no further note of the scale: stay at the last one
        }
        at = next;
    }
    return at;
}

} // namespace pnem::transpose
