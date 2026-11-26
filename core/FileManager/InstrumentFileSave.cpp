// PNEM CE: Save instrument (see InstrumentFileSave.h).

#include "InstrumentFileSave.h"

#include "fileManager.h"
#include "fileManagerDefs.h"
#include "fileTransfer.h"

#include <cstdio>
#include <cstring>
#include <strings.h>

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

    // Where the sample data is: the workspace instrument file, or the project's when the workspace
    // one holds only the settings (as the Tracker itself resolves it when loading)
    snprintf(sourcePath, sizeof(sourcePath), cWorkspaceInstrumentFileFormatNew, idx + 1);
    uint32_t offset = sizeof(strInstrumentFile);
    bool found = fileSystem->exists(sourcePath) &&
                 fileSystem->getFileSize(sourcePath) > sizeof(strInstrumentFile) &&
                 newFileManager.getPtiFileDataOffset(sourcePath, offset);
    if (!found)
    {
        offset = sizeof(strInstrumentFile);
        found = newFileManager.resolveProjectInstrumentPtiPath(idx, sourcePath, sizeof(sourcePath)) &&
                newFileManager.getPtiFileDataOffset(sourcePath, offset);
    }
    if (!found)
    {
        return false;
    }
    const uint32_t size = fileSystem->getFileSize(sourcePath);
    auto withoutRoot = [](const char* p) { return p[0] == '/' ? p + 1 : p; };
    if (size <= offset || strcasecmp(withoutRoot(sourcePath), withoutRoot(destPath)) == 0)
    {
        return false; // no sample data, or the file the instrument is kept in itself
    }
    sourceDataOffset = offset;
    dataBytes = size - offset;

    // A file that is there already was confirmed by the user: start from an empty one
    if (fileSystem->exists(destPath))
    {
        fileSystem->remove(destPath);
    }

    // The settings as they are now, under the name the file gets (the Sample Loader names the
    // instrument it loads after it)
    strInstrument instrument;
    memcpy(&instrument, &mtProject.instrument[idx], sizeof(instrument));
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
    newFileManager.writeInstrumentToFileStruct(&instrument, &header);
    fileTransfer.clearBytesComplited();
    step = Step::header;
    newFileManager.setStatus(fmSavingInstrumentToFile);
    newFileManager.setCurrentOperation(fmSaveInstrumentToFile);
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
                step = Step::sampleData;
            }
            else if (status >= fileTransferError)
            {
                fail();
            }
            break;
        }
        case Step::sampleData:
        {
            const uint8_t status = fileTransfer.copyRawDataBetweenFiles(
              sourcePath, sourceDataOffset, destPath, sizeof(strInstrumentFile), dataBytes);
            if (status == fileTransferEnd)
            {
                step = Step::finish;
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
    if (step == Step::header || dataBytes == 0)
    {
        return 0.0f;
    }
    if (step == Step::finish)
    {
        return 1.0f;
    }
    const uint32_t done = fileTransfer.getBytesComplited();
    return done >= dataBytes ? 1.0f : (float)done / dataBytes;
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
