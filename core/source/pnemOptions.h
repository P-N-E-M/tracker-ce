// PNEM CE: options in Config > General for this fork's Pattern features.
//
// They live in the one padding byte mtConfig.general already had (the config format stays as
// CE's): a set bit turns an option off, so a config CE wrote (0 there) has them all on. The menu
// items edit one byte each (optionMenuValue, 1 = On); optionChanged writes them back, and
// optionsFromConfig fills them after the config is read.

#ifndef PNEM_OPTIONS_H_
#define PNEM_OPTIONS_H_

#include <stdint.h>

namespace pnem
{

enum Option : uint8_t
{
    kEightTrackView, // Pattern opens with 8 tracks (Note + Instr); a Pattern tap toggles 8 / 4
    kTrackWrap,      // Left on track 1 goes to track 16, Right on track 16 to track 1
    kAlignMuteKeys,  // Shift + softkey mutes / arms the tracks on screen
    kOptionCount
};

bool option(Option which);

extern uint8_t optionMenuValue[kOptionCount];
void optionsFromConfig();
void optionChanged();

} // namespace pnem

#endif /* PNEM_OPTIONS_H_ */
