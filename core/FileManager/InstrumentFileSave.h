// PNEM CE: Save instrument (Sample Loader > More > Save instrument).
//
// Writes one sample instrument as a .pti file at a path the user chose: the instrument's settings as
// they are now (from memory, as a project save writes them; its name the file's, without .pti),
// followed by its sample data, copied
// byte for byte from the file the Tracker keeps it in (the workspace instrument file, or the
// project's when the workspace one holds only the settings). The result is the same kind of file
// a project keeps in its instruments folder, and the Sample Loader loads it with all its settings.

#ifndef FILEMANAGER_INSTRUMENTFILESAVE_H_
#define FILEMANAGER_INSTRUMENTFILESAVE_H_

#include "FileManagerOperation.h"
#include "fileManagerDefs.h"

#include <stdint.h>

class InstrumentFileSave : public FileManagerOperation
{
  public:
    // Starts the operation (the file manager must be idle); false if it can't: no such instrument,
    // no sample data found, the path too long, or the path is the instrument's own backing file
    bool start(int instrumentIdx, const char* destPath);

    virtual void update() override;
    virtual float getProgress() override;

  private:
    enum class Step : uint8_t
    {
        header,
        sampleData,
        finish,
    };

    void fail();

    Step step = Step::header;
    int idx = 0;
    char destPath[PATCH_SIZE];
    char sourcePath[PATCH_SIZE];
    uint32_t sourceDataOffset = 0;
    uint32_t dataBytes = 0;
    strInstrumentFile header;
};

#endif /* FILEMANAGER_INSTRUMENTFILESAVE_H_ */
