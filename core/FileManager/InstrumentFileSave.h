// PNEM CE: Save instrument and Build kit (Sample Loader > More).
//
// Save instrument writes one sample instrument as a .pti file at a path the user chose: the
// instrument's settings as they are now (from memory, as a project save writes them; its name the
// file's, without .pti), followed by its sample data, copied byte for byte from the file the Tracker
// keeps it in (the workspace instrument file, or the project's when the workspace one holds only the
// settings). The result is the same kind of file a project keeps in its instruments folder, and the
// Sample Loader loads it with all its settings.
//
// Build kit writes a run of instruments as one Beat Slice instrument: each instrument's sample from
// its start point to its end point, one after the other, with a slice at the start of each. Between
// two of them sits a little silence, wider than a slice point's step (a slice point is a 16-bit
// fraction of the whole sample), and the slice point lies in its middle: a slice never starts in the
// tail of the one before or cuts into an attack. Stereo if any of them is (a mono one in both
// channels); the kit has the default settings, in Beat Slice mode.

#ifndef FILEMANAGER_INSTRUMENTFILESAVE_H_
#define FILEMANAGER_INSTRUMENTFILESAVE_H_

#include "FileManagerOperation.h"
#include "fileManagerDefs.h"

#include <stdint.h>

class InstrumentFileSave : public FileManagerOperation
{
  public:
    static constexpr uint8_t kMaxParts = MAX_SLICE_NUMBER;

    // Starts the operation (the file manager must be idle); false if it can't: no such instrument,
    // no sample data found, the path too long, or the path is the instrument's own backing file
    bool start(int instrumentIdx, const char* destPath);

    // Build kit: the active instruments from `first` to `last` (at least two, each with sample
    // data). False as start, or for fewer than two.
    bool startKit(int first, int last, const char* destPath);

    virtual void update() override;
    virtual float getProgress() override;

  private:
    enum class Step : uint8_t
    {
        header,
        pieces,
        finish,
    };

    struct Part
    {
        uint8_t slot;
        bool stereo;
        uint32_t fileFrames; // frames per channel in the instrument's file
        uint32_t firstFrame; // its start point
        uint32_t frames;     // to its end point
    };

    bool sourceOf(int instrument, char* path, uint32_t& dataOffset) const;
    bool isOwnSource(const char* path) const;
    void nameAfterFile(strInstrument& instrument) const;
    void begin(uint16_t pieces, uint32_t bytes);
    bool startPiece();
    void fail();

    Step step = Step::header;
    bool kit = false;
    int idx = 0;

    // The kit
    uint8_t partCount = 0;
    Part parts[kMaxParts];
    bool stereoOut = false;
    uint32_t gapFrames = 0;

    // The pieces after the header: one copy for an instrument; for a kit, per channel, each part
    // and the silence after it
    uint16_t piece = 0;
    uint16_t pieceCount = 0;
    bool pieceStarted = false;
    bool pieceIsGap = false;
    uint32_t pieceSourceOffset = 0;
    uint32_t pieceBytes = 0;
    uint32_t destOffset = 0;
    uint32_t totalBytes = 0;

    char destPath[PATCH_SIZE];
    char sourcePath[PATCH_SIZE];
    uint32_t sourceDataOffset = 0;
    uint32_t dataBytes = 0;
    strInstrumentFile header;
};

#endif /* FILEMANAGER_INSTRUMENTFILESAVE_H_ */
