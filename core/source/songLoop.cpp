// PNEM CE: Song Loop.

#include "songLoop.h"

#include "config.h"

#define SL_LOCK() DISABLE_GLOBAL_INTERRUPTS()
#define SL_UNLOCK() ENABLE_GLOBAL_INTERRUPTS()

namespace songloop
{

namespace
{

struct State
{
    uint8_t active;  // a block loops, or waits for the song to get there
    uint8_t entered; // the song has played inside the block since the loop started
    uint8_t first;
    uint8_t last;
    uint8_t pending;     // a preload decision for the pattern at pendingFrom
    uint8_t pendingFrom; // the position it was made at
    uint8_t pendingPos;  // the position the pattern end goes to
};

volatile State state;

// The Song screen's list block (main loop only).
uint8_t listBlockFirst;
uint8_t listBlockLength;

} // namespace

int16_t nextPattern(const Song& song)
{
    const uint8_t pos = *song.playlistPos;
    if (state.pending && state.pendingFrom == pos)
    {
        return song.playlist[state.pendingPos];
    }
    if (!state.active)
    {
        return -1;
    }

    uint8_t next;
    if (pos >= state.first && pos <= state.last)
    {
        state.entered = 1;
        next = pos < state.last ? pos + 1 : state.first;
    }
    else if (!state.entered && pos < state.first)
    {
        return -1; // the block is ahead: the song plays on
    }
    else
    {
        next = state.first; // the block is behind, or was moved away from the play position
    }
    if (song.playlist[next] == 0)
    {
        return -1;
    }
    state.pendingFrom = pos;
    state.pendingPos = next;
    state.pending = 1;
    return song.playlist[next];
}

int8_t switchNext(const Song& song)
{
    const bool ours = state.pending && state.pendingFrom == *song.playlistPos;
    state.pending = 0;
    if (!ours)
    {
        return -1;
    }
    *song.playlistPos = state.pendingPos;
    return 1;
}

bool keepPerformance()
{
    return state.active;
}

uint8_t leftLabel(bool stopped, bool songMode, bool blockSelected)
{
    if (state.active && !stopped)
    {
        return 2;
    }
    return blockSelected && (stopped || songMode) ? 1 : 0;
}

bool press(bool stopped, bool songMode, uint8_t first, uint8_t last)
{
    if (state.active)
    {
        // Continue Song: the block plays out (once more if its next pattern is already
        // loading), then the song goes on
        SL_LOCK();
        state.active = 0;
        state.entered = 0;
        SL_UNLOCK();
        return true;
    }
    if (!stopped && !songMode)
    {
        return false; // a pattern loop: CE's Continue Song first
    }
    SL_LOCK();
    state.first = first;
    state.last = last;
    state.entered = 0;
    state.active = 1;
    SL_UNLOCK();
    return true;
}

void setBlock(uint8_t first, uint8_t last)
{
    SL_LOCK();
    state.first = first;
    state.last = last;
    SL_UNLOCK();
}

void setListBlock(uint8_t first, uint8_t length)
{
    listBlockFirst = first;
    listBlockLength = length;
}

uint8_t listFirst()
{
    return listBlockFirst;
}

uint8_t listLength()
{
    return listBlockLength;
}

bool listFrame(uint8_t* first, uint8_t* last)
{
    const bool shown = state.active ? listBlockLength >= 1 : listBlockLength >= 2;
    if (!shown)
    {
        return false;
    }
    *first = listBlockFirst;
    *last = listBlockFirst + listBlockLength - 1;
    return true;
}

bool active()
{
    return state.active;
}

bool looping(uint8_t pos)
{
    return state.active && pos >= state.first && pos <= state.last;
}

bool armed(uint8_t pos)
{
    return state.active && (pos < state.first || pos > state.last);
}

namespace
{

// The font's L4 glyph cells are 18 x 24 px (9 bytes a row); a sign is a pair of cells.
constexpr int kCellWidth = 18;
constexpr int kCellHeight = 24;
constexpr int kStride = 9;
constexpr int kHeader = 148; // widths[128], then format, stride, width, height, pointer

// Fills x0..x1, y0..y1 of the 36 px pair at its first glyph.
void fill(uint8_t* left, uint8_t* right, int x0, int x1, int y0, int y1)
{
    for (int y = y0; y <= y1; y++)
    {
        for (int x = x0; x <= x1; x++)
        {
            uint8_t* cell = x < kCellWidth ? left : right;
            const int cx = x < kCellWidth ? x : x - kCellWidth;
            uint8_t& b = cell[y * kStride + cx / 2];
            b = cx % 2 == 0 ? static_cast<uint8_t>((b & 0x0F) | 0xF0) : static_cast<uint8_t>((b & 0xF0) | 0x0F);
        }
    }
}

// A bar and a thin bar with two dots beside them, facing right (|:) or left (:|), from x.
void repeatSign(uint8_t* left, uint8_t* right, int x, bool facingRight)
{
    // rows 7..18 as CE's transport icons; dots at the thirds
    const int thick = facingRight ? x : x + 5;
    const int thin = facingRight ? x + 3 : x + 3;
    const int dots = facingRight ? x + 5 : x;
    fill(left, right, thick, thick + 1, 7, 18);
    fill(left, right, thin, thin, 7, 18);
    fill(left, right, dots, dots + 1, 9, 10);
    fill(left, right, dots, dots + 1, 15, 16);
}

} // namespace

void patchFont(uint8_t* font)
{
    const uint32_t* metrics = reinterpret_cast<const uint32_t*>(font + 128);
    if (metrics[0] != 2 /* L4 */ || metrics[1] != kStride || metrics[3] != kCellHeight)
    {
        return;
    }
    int firstChar = 0;
    while (firstChar < 127 && font[firstChar] == 0)
    {
        firstChar++;
    }
    const char glyphs[] = { kArmedSign[0], kArmedSign[1], kLoopSign[0], kLoopSign[1] };
    uint8_t* cells[4];
    for (int i = 0; i < 4; i++)
    {
        const int c = glyphs[i];
        if (c < firstChar || font[c] == 0)
        {
            return;
        }
        cells[i] = font + kHeader + (c - firstChar) * kStride * kCellHeight;
    }
    for (int i = 0; i < 4; i++)
    {
        font[static_cast<int>(glyphs[i])] = kCellWidth;
        for (int b = 0; b < kStride * kCellHeight; b++)
        {
            cells[i][b] = 0;
        }
    }
    // |: centred where CE's play icon is (columns 12..21 of the pair)
    repeatSign(cells[0], cells[1], 13, true);
    // |: :| in 20 columns, 8..27
    repeatSign(cells[2], cells[3], 8, true);
    repeatSign(cells[2], cells[3], 21, false);
}

void songPositionSet()
{
    state.pending = 0;
}

void stopped()
{
    state.active = 0;
    state.entered = 0;
    state.pending = 0;
}

} // namespace songloop
