// PNEM CE: Song Loop.
//
// In the Song screen's pattern list, Shift + Down selects a block of song positions
// downwards from the cursor (Shift + Up takes the bottom back; one frame over the block's
// rows) and the left softkey reads Play Selection: the song repeats the block while you jam
// over it. Continue Song plays the block out and goes on with the song.
//
//  - Stopped: the song plays from the block's first position and repeats the block.
//  - Playing the song, block ahead: the song plays on and repeats the block once it gets
//    there (title bar |:). Block behind: the song goes to the block's start at the pattern
//    end. While it loops the title bar and the list show |: :|.
//  - During the loop Up/Down and the jog move the whole block, Shift + Up/Down resize it; a
//    play position outside the block goes to its start at the pattern end. Edits that change
//    the song's slots (Add Slot, Delete Slot, << Pattern >>) do nothing while a block is
//    selected or loops; Undo and the song grid's Delete, Copy and Paste work, and the grid's
//    selection (CE's, with Rec) leaves the block alone. The block's frame turns grey while
//    the cursor is in the grid.
//  - Play Song with a block plays the song from the block's start and drops the block.
//  - Performance stays on for every pattern of the block, as Loop Performance does for
//    one pattern; after Continue Song it goes off at the next pattern end, as in CE.
//  - Stop, anywhere, ends the loop. Nothing is saved.
//
// The sequencer asks for the next pattern once per pattern (nextPattern, its preload)
// and moves the play position at the pattern end (switchNext). The position chosen at
// the preload is kept, so the loaded pattern and the position always match, also when
// the block changes in between. Both run in the audio interrupt: state in RAM, no SD,
// heap or UI work. The block is set from the main loop with interrupts off.

#ifndef SONG_LOOP_H_
#define SONG_LOOP_H_

#include <cstdint>

namespace songloop
{

// The playlist as CE keeps it: playlist[pos] is a pattern, 0 an empty slot (the end).
struct Song
{
    const uint8_t* playlist;
    uint8_t* playlistPos;
};

// --- audio interrupt ---

// The pattern to preload, or -1 to let CE decide.
int16_t nextPattern(const Song& song);

// The pattern end: moves *song.playlistPos and returns 1, as CE's
// switchNextPatternInSong does (0 when the song ends and starts over); -1 lets CE do it.
int8_t switchNext(const Song& song);

// At a pattern end: whether Performance's scheduled turn-off waits (the block loops).
bool keepPerformance();

// --- main loop ---

// The left softkey: 1 Play Selection, 2 Continue Song, 0 CE's own label.
// stopped/songMode: the sequencer's state; blockSelected: two or more positions
// selected in the pattern list.
uint8_t leftLabel(bool stopped, bool songMode, bool blockSelected);

// The left softkey was pressed with leftLabel() != 0: true if Song Loop handled it.
// first/last: the selected positions. Starting from stop is left to the caller (it plays
// from first, then calls this again).
bool press(bool stopped, bool songMode, uint8_t first, uint8_t last);

// The block moved or changed size during the loop.
void setBlock(uint8_t first, uint8_t last);

// The block selected in the Song screen's pattern list (main loop only): first position and
// length, 0 for none. During the loop it is the looping block.
void setListBlock(uint8_t first, uint8_t length);
uint8_t listFirst();
uint8_t listLength();

// For the pattern list's frame: the rows to frame, when a block of two or more is selected
// or a block loops. It stays while the cursor is in the song grid.
bool listFrame(uint8_t* first, uint8_t* last);

// The repeat signs, two glyphs each in the 17 px bold font (handle 4): "|:" while the song
// heads for the block, "|: :|" while it loops. patchFont draws them over four glyphs no
// screen uses (^ ` { }); a font with other metrics is left alone.
constexpr char kArmedSign[] = "^`";
constexpr char kLoopSign[] = "{}";
constexpr uint8_t kSignFontIndex = 3; // fonts[3], handle 4
void patchFont(uint8_t* font);

// The pattern list's icon on the playing position, while a block is set: the loop sign
// inside it, the opening sign while the song heads for it (instead of CE's play icon).
constexpr uint8_t kListIcon = 0xFE;
constexpr uint8_t kArmedListIcon = 0xFD;

// Whether a block loops or waits to (the song screen then protects the song).
bool active();

// Whether the play position is inside the looping block (the repeat signs).
bool looping(uint8_t pos);

// Whether the song heads for the block: a loop is set and the play position is outside it.
bool armed(uint8_t pos);

// CE set a song position (play, loop or continue): a preload decision is stale.
void songPositionSet();

// Stop: the loop ends.
void stopped();

} // namespace songloop

#endif /* SONG_LOOP_H_ */
