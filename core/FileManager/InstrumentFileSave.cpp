// PNEM CE: Save instrument and Build kit (see InstrumentFileSave.h).

#include "InstrumentFileSave.h"

#include "fileManager.h"
#include "fileManagerDefs.h"
#include "fileTransfer.h"

#include <cstdio>
#include <cstring>
#include <strings.h>

namespace
{
// The silence between the parts of a kit
const uint8_t kSilence[4096] = {};
constexpr uint32_t kMaxGapFrames = sizeof(kSilence) / sizeof(int16_t);
constexpr uint32_t kMinGapFrames = 8;

const char* withoutRoot(const char* path)
{
    return path[0] == '/' ? path + 1 : path;
}
} // namespace

// Where an instrument's sample data is: the workspace instrument file, or the project's when the
// workspace one holds only the settings (as the Tracker itself resolves it when loading)
bool InstrumentFileSave::sourceOf(int instrument, char* path, uint32_t& dataOffset) const
{
    snprintf(path, PATCH_SIZE, cWorkspaceInstrumentFileFormatNew, instrument + 1);
    dataOffset = sizeof(strInstrumentFile);
    if (fileSystem->exists(path) && fileSystem->getFileSize(path) > sizeof(strInstrumentFile) &&
        newFileManager.getPtiFileDataOffset(path, dataOffset))
    {
        return fileSystem->getFileSize(path) > dataOffset;
    }
    dataOffset = sizeof(strInstrumentFile);
    return newFileManager.resolveProjectInstrumentPtiPath(instrument, path, PATCH_SIZE) &&
           newFileManager.getPtiFileDataOffset(path, dataOffset) &&
           fileSystem->getFileSize(path) > dataOffset;
}

// The file being written is a file an instrument is kept in (FAT doesn't mind case)
bool InstrumentFileSave::isOwnSource(const char* path) const
{
    return strcasecmp(withoutRoot(path), withoutRoot(destPath)) == 0;
}

// The instrument named after the file (without .pti): the Sample Loader names what it loads so
void InstrumentFileSave::nameAfterFile(strInstrument& instrument) const
{
    const char* slash = strrchr(destPath, '/');
    const char* base = slash != nullptr ? slash + 1 : destPath;
    size_t nameLen = strlen(base);
    if (nameLen > 4 && strcasecmp(base + nameLen - 4, ".pti") == 0)
    {
        nameLen -= 4;
    }
    if (nameLen > sizeof(instrument.sample.file_name) - 1)
    {
        nameLen = sizeof(instrument.sample.file_name) - 1;
    }
    memcpy(instrument.sample.file_name, base, nameLen);
    instrument.sample.file_name[nameLen] = 0;
}

bool InstrumentFileSave::start(int instrumentIdx, const char* path)
{
    if (newFileManager.getStatus() != fmIdle || newFileManager.getCurrentOperation() != fmNoOperation)
    {
        return false;
    }
    if (instrumentIdx < 0 || instrumentIdx >= kSampleInstrumentsCount ||
        !mtProject.instrument[instrumentIdx].isActive || strlen(path) >= sizeof(destPath))
    {
        return false;
    }
    idx = instrumentIdx;
    strcpy(destPath, path);
    if (!sourceOf(idx, sourcePath, sourceDataOffset) || isOwnSource(sourcePath))
    {
        return false; // no sample data, or the file the instrument is kept in itself
    }
    dataBytes = fileSystem->getFileSize(sourcePath) - sourceDataOffset;

    // The settings as they are now, under the name the file gets
    strInstrument instrument;
    memcpy(&instrument, &mtProject.instrument[idx], sizeof(instrument));
    nameAfterFile(instrument);
    newFileManager.writeInstrumentToFileStruct(&instrument, &header);

    kit = false;
    begin(1, dataBytes);
    return true;
}

bool InstrumentFileSave::startKit(int first, int last, const char* path)
{
    if (newFileManager.getStatus() != fmIdle || newFileManager.getCurrentOperation() != fmNoOperation)
    {
        return false;
    }
    if (first < 0 || last >= kSampleInstrumentsCount || first > last || strlen(path) >= sizeof(destPath))
    {
        return false;
    }
    strcpy(destPath, path);

    // The parts: each active instrument's sample from its start point to its end point
    partCount = 0;
    stereoOut = false;
    uint32_t sum = 0;
    for (int i = first; i <= last; i++)
    {
        const strInstrument& instrument = mtProject.instrument[i];
        if (!instrument.isActive)
        {
            continue;
        }
        char source[PATCH_SIZE];
        uint32_t offset;
        const uint32_t frames = instrument.sample.length;
        if (partCount >= kMaxParts || frames == 0 || !sourceOf(i, source, offset) || isOwnSource(source))
        {
            return false;
        }
        const uint32_t bytes = fileSystem->getFileSize(source) - offset;
        Part& part = parts[partCount++];
        part.slot = (uint8_t)i;
        if (bytes >= frames * 4)
        {
            part.stereo = true;
        }
        else if (bytes >= frames * 2)
        {
            part.stereo = false;
        }
        else
        {
            return false; // the file doesn't hold the sample the instrument says it has
        }
        uint32_t from = (uint32_t)((uint64_t)instrument.startPoint * frames / MAX_16BIT);
        uint32_t to = (uint32_t)((uint64_t)instrument.endPoint * frames / MAX_16BIT);
        if (to > frames)
        {
            to = frames;
        }
        if (to <= from)
        {
            from = 0;
            to = frames;
        }
        part.fileFrames = frames;
        part.firstFrame = from;
        part.frames = to - from;
        stereoOut = stereoOut || part.stereo;
        sum += part.frames;
    }
    if (partCount < 2)
    {
        return false;
    }

    // The silence between the parts: two slice point steps, and a little
    uint32_t gap = kMinGapFrames;
    for (int i = 0; i < 3; i++)
    {
        const uint32_t total = sum + gap * (partCount - 1);
        gap = 2 * ((total + MAX_16BIT - 1) / MAX_16BIT) + kMinGapFrames;
    }
    if (gap > kMaxGapFrames)
    {
        return false; // a kit of half an hour and more
    }
    gapFrames = gap;
    const uint32_t total = sum + gap * (partCount - 1);

    // A Beat Slice instrument with the default settings, a slice in the middle of each silence
    strInstrument instrument;
    newFileManager.setDefaultActiveInstrument(&instrument);
    instrument.sample.type = mtSampleTypeWaveFile;
    instrument.sample.length = total;
    instrument.playMode = playModeBeatSlice;
    instrument.sliceNumber = partCount;
    instrument.selectedSlice = 0;
    uint32_t partStart = 0;
    for (uint8_t k = 0; k < partCount; k++)
    {
        const uint32_t at = k == 0 ? 0 : partStart - gap / 2;
        instrument.slices[k] = (uint16_t)(((uint64_t)at * MAX_16BIT + total / 2) / total);
        partStart += parts[k].frames + gap;
    }
    nameAfterFile(instrument);
    newFileManager.writeInstrumentToFileStruct(&instrument, &header);

    kit = true;
    const uint8_t channels = stereoOut ? 2 : 1;
    begin(channels * (2 * partCount - 1), total * sizeof(int16_t) * channels);
    return true;
}

void InstrumentFileSave::begin(uint16_t pieces, uint32_t bytes)
{
    // A file that is there already was confirmed by the user: start from an empty one
    if (fileSystem->exists(destPath))
    {
        fileSystem->remove(destPath);
    }
    pieceCount = pieces;
    piece = 0;
    pieceStarted = false;
    totalBytes = bytes;
    destOffset = sizeof(strInstrumentFile);
    fileTransfer.clearBytesComplited();
    step = Step::header;
    newFileManager.setStatus(fmSavingInstrumentToFile);
    newFileManager.setCurrentOperation(fmSaveInstrumentToFile);
}

// The next piece: an instrument's data; for a kit, per channel, a part (a mono one's data in both
// channels) or the silence after it
bool InstrumentFileSave::startPiece()
{
    if (!kit)
    {
        pieceIsGap = false;
        pieceSourceOffset = sourceDataOffset;
        pieceBytes = dataBytes;
        return true;
    }
    const uint16_t perChannel = 2 * partCount - 1;
    const uint16_t channel = piece / perChannel;
    const uint16_t inChannel = piece % perChannel;
    if (inChannel % 2 == 1)
    {
        pieceIsGap = true;
        pieceBytes = gapFrames * sizeof(int16_t);
        return true;
    }
    const Part& part = parts[inChannel / 2];
    uint32_t offset;
    if (!sourceOf(part.slot, sourcePath, offset))
    {
        return false;
    }
    pieceIsGap = false;
    pieceSourceOffset = offset + (channel == 1 && part.stereo ? part.fileFrames * sizeof(int16_t) : 0) +
                        part.firstFrame * sizeof(int16_t);
    pieceBytes = part.frames * sizeof(int16_t);
    return true;
}

void InstrumentFileSave::update()
{
    switch (step)
    {
        case Step::header:
        {
            uint32_t written = 0;
            const uint8_t status = fileTransfer.saveInstrumentStructToInstrumentFile(
              (uint8_t*)&header, sizeof(header), destPath, &written);
            if (status == fileTransferEnd)
            {
                step = Step::pieces;
            }
            else if (status >= fileTransferError)
            {
                fail();
            }
            break;
        }
        case Step::pieces:
        {
            if (!pieceStarted)
            {
                if (!startPiece())
                {
                    fail();
                    break;
                }
                pieceStarted = true;
            }
            const uint8_t status =
              pieceIsGap ? fileTransfer.saveSampleRawDataToFile(
                             const_cast<uint8_t*>(kSilence), pieceBytes, destPath, destOffset)
                         : fileTransfer.copyRawDataBetweenFiles(
                             sourcePath, pieceSourceOffset, destPath, destOffset, pieceBytes);
            if (status == fileTransferEnd)
            {
                destOffset += pieceBytes;
                pieceStarted = false;
                if (++piece >= pieceCount)
                {
                    step = Step::finish;
                }
            }
            else if (status >= fileTransferError)
            {
                fail();
            }
            break;
        }
        case Step::finish:
        {
            newFileManager.setStatus(fmSaveInstrumentEnd);
            newFileManager.setCurrentOperation(fmNoOperation);
            break;
        }
    }
}

float InstrumentFileSave::getProgress()
{
    if (step == Step::header || totalBytes == 0)
    {
        return 0.0f;
    }
    if (step == Step::finish)
    {
        return 1.0f;
    }
    const uint32_t done = fileTransfer.getBytesComplited();
    return done >= totalBytes ? 1.0f : (float)done / totalBytes;
}

void InstrumentFileSave::fail()
{
    fileSystem->remove(destPath); // no half file left behind
    // Not stopOperationWithError: that restarts the audio engine (clearing reverb and delay) for
    // operations that stop it, and this one leaves it running
    newFileManager.setStatus(fmSaveInstrumentError);
    newFileManager.resetCurrentOperationStep();
    newFileManager.setCurrentOperation(fmNoOperation);
}
