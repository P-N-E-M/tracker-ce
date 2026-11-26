// PNEM CE: Save instrument in the Sample Loader.
//
// The button that said Delete unused now says More (with two dots, as in Sample Playback's Slice): it
// toggles a second row of buttons, Delete unused and Save instrument, while the lists above work as
// before. Save instrument saves the instrument selected in the list as a .pti file, with its sample
// and all its settings, in a folder the user picks on a screen like Save Project's (the folder's
// .pti and .wav files shown too): Enter, New Folder, Cancel, Save, then the name on the keyboard. The
// Sample Loader loads such a file back with all its settings, under that name.

#include "sampleImporter/sampleImporter.h"

#include "FileBrowser.h"
#include "Platform.h"
#include "core/interfacePopups.h"
#include "fileManager.h"
#include "mtAudioEngine.h"
#include "util_string.h"

#include <cstdio>
#include <cstring>

extern uint8_t externalRamScratchBuff[];

namespace
{
cSampleImporter* const SI = &sampleImporter;

constexpr const char kInstrumentExtension[] = ".pti";

FileBrowser browser;
FileBrowserData browserData;
bool browsing = false;
char lastPath[kMaxFilePathLen + 1] = ""; // the folder the last instrument went to ("": the root)
char destPath[PATCH_SIZE];
uint8_t saveSlot = 0;
uint8_t placeBefore = 0;

void noButton() {}
void noShift(uint8_t) {} // Shift selects ranges in the Sample Loader's lists

void functBrowserEnter();

// The encoder: nothing (the Mini confirms a key with it, as in Rename)
void noPot(int16_t)
{
#ifdef TRACKER_MINI
    if (SI->keyboardManager.getState())
    {
        SI->keyboardManager.confirmKey();
    }
#endif
}

// The Mini's middle button: a key on the keyboard, Enter in the folder list
void miniMiddle(uint8_t state)
{
    if (state != buttonPress)
    {
        return;
    }
    if (SI->keyboardManager.getState())
    {
        SI->keyboardManager.confirmKey();
    }
    else if (browsing)
    {
        functBrowserEnter();
    }
}

// The labels: texts (two lines), active or not
void setLabel(uint8_t idx, const char* text, const char* text2, bool active)
{
    display.setControlText(SI->label[idx], text);
    display.setControlText2(SI->label[idx], text2);
    display.setControlColors(SI->label[idx],
                             active ? display.getTheme().activeButtonLabelsColors
                                    : display.getTheme().inactiveButtonLabelsColors);
    display.setControlValue(SI->label[idx], 0);
    display.setControlStyle(SI->label[idx],
                            controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
    display.setControlStyle2(SI->label[idx],
                             controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
    display.setControlPosition(SI->label[idx], (800 / 8) * idx + (800 / 16), 424);
    display.setControlSize(SI->label[idx], 800 / 8 - 6, 55);
    display.setControlShow(SI->label[idx]);
    display.refreshControl(SI->label[idx]);
}

void clearLabels()
{
    for (uint8_t i = 0; i < 8; i++)
    {
        setLabel(i, "", "", true);
    }
}

// Everything but the buttons this screen sets does nothing
void blockOtherControls()
{
    SI->ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    SI->ip->setPotObj(interfacePot0, noPot);
    SI->ip->setButtonObj(interfaceButtonUp, buttonPress, noButton);
    SI->ip->setButtonObj(interfaceButtonDown, buttonPress, noButton);
    SI->ip->setButtonObj(interfaceButtonLeft, buttonPress, noButton);
    SI->ip->setButtonObj(interfaceButtonRight, buttonPress, noButton);
    SI->ip->setButtonObj(interfaceButtonInsert, buttonPress, noButton);
    SI->ip->setButtonObj(interfaceButtonCopy, buttonPress, noButton);
    SI->ip->clearButton(interfaceButtonDelete);
    SI->ip->setButtonObj(interfaceButtonShift, noShift);
    SI->ip->setButtonObj(interfaceButtonMiniMiddle, miniMiddle);
}

//------------------------------------------------------------------------------------------------
// The folder browser

void showFolders()
{
    SI->explorerList.data = browserData.browserList;
    SI->explorerList.length = browserData.browserListLength;
    SI->explorerList.start = browserData.browserListPosition;
    SI->explorerList.selectTab = nullptr;
    display.setControlData(SI->explorerListControl, &SI->explorerList);
    display.setControlValue(SI->explorerListControl, browserData.browserListPosition);
    display.setControlShow(SI->explorerListControl);
    display.refreshControl(SI->explorerListControl);
}

void moveFolderSelection(int16_t value)
{
    if (browserData.browserListLength <= 0)
    {
        return;
    }
    browserData.browserListPosition =
      clamp<int>(browserData.browserListPosition + clamp<int>(value, -5, 5), 0,
                 browserData.browserListLength - 1);
    display.setControlValue(SI->explorerListControl, browserData.browserListPosition);
    display.refreshControl(SI->explorerListControl);
}

void functBrowserPot(int16_t value)
{
    moveFolderSelection(interfaceGlobals.invertValForList ? -value : value);
}

void functBrowserUp()
{
    moveFolderSelection(-1);
}

void functBrowserDown()
{
    moveFolderSelection(1);
}

void functBrowserEnter()
{
    browser.browse();
    showFolders();
}

void functBrowserNewFolder();
void functBrowserSave();
void functBrowserCancel();

void showBrowser()
{
    SI->keyboardManager.deactivateKeyboard();
    mtPopups.hideInfoPopup();

    SI->titleBar->show();
    SI->titleBar->setTitle("Sample Loader - Save Instrument");

    display.setControlText(SI->explorerLabelControl, "SD Card");
    display.setControlShow(SI->explorerLabelControl);
    display.refreshControl(SI->explorerLabelControl);
    display.setControlText(SI->instrumentLabelControl, "Instruments");
    display.setControlShow(SI->instrumentLabelControl);
    display.refreshControl(SI->instrumentLabelControl);

    SI->showInstrumentsList();
    showFolders();
    display.setControlHide(SI->memoryBarControl);
    display.refreshControl(SI->memoryBarControl);
    display.setControlHide(SI->previewVolumeBarControl);
    display.refreshControl(SI->previewVolumeBarControl);

    display.setControlShow(SI->frameControl);
    display.refreshControl(SI->frameControl);
    SI->selectedPlace = 0;
    SI->activateLabelsBorder();

    clearLabels();
    setLabel(0, "Enter", "", true);
    setLabel(3, "New Folder", "", true);
    setLabel(6, "Cancel", "", true);
    setLabel(7, "Save", "", true);
    display.setControlValue(SI->bgLabel, 255);
    display.refreshControl(SI->bgLabel);

    blockOtherControls();
    SI->ip->setPotObj(interfacePot0, functBrowserPot);
    SI->ip->setButtonObj(interfaceButtonUp, buttonPress, functBrowserUp);
    SI->ip->setButtonObj(interfaceButtonDown, buttonPress, functBrowserDown);
    SI->ip->setButtonObj(interfaceButtonInsert, buttonPress, functBrowserEnter);
    SI->ip->setButtonObj(interfaceButton0, buttonPress, functBrowserEnter);
    SI->ip->setButtonObj(interfaceButton3, buttonPress, functBrowserNewFolder);
    SI->ip->setButtonObj(interfaceButton6, buttonPress, functBrowserCancel);
    SI->ip->setButtonObj(interfaceButton7, buttonPress, functBrowserSave);

    display.synchronizeRefresh();
}

//------------------------------------------------------------------------------------------------
// The keyboard (a new folder's name, the instrument's file name)

void functKeyboardLeft()
{
    SI->keyboardManager.makeMove('a');
}
void functKeyboardRight()
{
    SI->keyboardManager.makeMove('d');
}
void functKeyboardUp()
{
    SI->keyboardManager.makeMove('w');
}
void functKeyboardDown()
{
    SI->keyboardManager.makeMove('s');
}
void functKeyboardKey()
{
    SI->keyboardManager.confirmKey();
}
void functKeyboardBackspace(uint8_t state)
{
    if (state == buttonPress || state == buttonHold)
    {
        SI->keyboardManager.makeBackspace();
    }
}
void functKeyboardAutoName()
{
    SI->keyboardManager.setRandomName();
}

void showKeyboard(const char* name, uint8_t maxLen, const char* confirmText, void (*confirm)())
{
    SI->keyboardManager.fillName(name);
    SI->keyboardManager.activateKeyboard(maxLen);
    SI->showRenameKeyboard();
    display.setControlText(SI->label[7], confirmText);
    display.refreshControl(SI->label[7]);
    display.synchronizeRefresh();

    blockOtherControls();
    SI->ip->setButtonObj(interfaceButtonLeft, buttonPress, functKeyboardLeft);
    SI->ip->setButtonObj(interfaceButtonRight, buttonPress, functKeyboardRight);
    SI->ip->setButtonObj(interfaceButtonUp, buttonPress, functKeyboardUp);
    SI->ip->setButtonObj(interfaceButtonDown, buttonPress, functKeyboardDown);
    SI->ip->setButtonObj(interfaceButtonInsert, buttonPress, functKeyboardKey);
    SI->ip->setButtonObj(interfaceButtonDelete, functKeyboardBackspace);
    SI->ip->setButtonObj(interfaceButton0, buttonPress, functKeyboardKey);
    SI->ip->setButtonObj(interfaceButton1, buttonPress, functKeyboardAutoName);
    SI->ip->setButtonObj(interfaceButton6, buttonPress, showBrowser);
    SI->ip->setButtonObj(interfaceButton7, buttonPress, confirm);
}

// The name typed, if it has anything but spaces
const char* typedName()
{
    const char* name = SI->keyboardManager.getName();
    for (const char* c = name; *c; c++)
    {
        if (*c != ' ')
        {
            return name;
        }
    }
    return nullptr;
}

// `name` in the current folder, with `extension`; false if the path gets too long
bool pathInCurrentFolder(const char* name, const char* extension, char* path, size_t size)
{
    const char* folder = browser.getCurrentPath();
    const int len = folder[0] == 0 ? snprintf(path, size, "%s%s", name, extension)
                                   : snprintf(path, size, "%s/%s%s", folder, name, extension);
    return len > 0 && (size_t)len < size;
}

void functNewFolderConfirm()
{
    const char* name = typedName();
    if (name == nullptr)
    {
        return;
    }
    char path[PATCH_SIZE];
    if (!pathInCurrentFolder(name, "", path, sizeof(path)))
    {
        mtPopups.show(1, "Maximum number of characters in folder path exceeded.");
    }
    else if (!browser.createFolderInCurrentPath((char*)name))
    {
        mtPopups.show(1, "A folder with the specified name already exists.");
    }
    else
    {
        browser.refresh();
        browser.selectSpecificFolderName((char*)name);
    }
    showBrowser();
}

void functBrowserNewFolder()
{
    showKeyboard("New folder", 32, "Create", functNewFolderConfirm);
}

void startSaving()
{
    mtPopups.hideInfoPopup();
    polystrncpy(lastPath, browser.getCurrentPath(), sizeof(lastPath));
    if (newFileManager.saveInstrumentToFile(saveSlot, destPath))
    {
        SI->ip->blockAllInputs(); // until cSampleImporter::update sees the end
    }
    else
    {
        mtPopups.show(1, "This instrument can't be saved here.");
        showBrowser();
    }
}

void functOverwriteNo()
{
    mtPopups.hideInfoPopup();
    SI->ip->unblockAllInputs();
    showKeyboard(SI->keyboardManager.getName(), SAMPLE_NAME_SIZE, "Save", functBrowserSave);
}

void functOverwriteYes()
{
    SI->ip->unblockAllInputs();
    startSaving();
}

void functSaveConfirm()
{
    const char* name = typedName();
    if (name == nullptr)
    {
        return;
    }
    if (!pathInCurrentFolder(name, kInstrumentExtension, destPath, sizeof(destPath)))
    {
        mtPopups.show(1, "Maximum number of characters in file path exceeded.");
        return;
    }
    if (SI->pnemFileSystem()->exists(destPath))
    {
        SI->keyboardManager.deactivateKeyboard();
        mtPopups.showInfoPopup("An instrument with this name already exists.", "Do you want to replace it?");
        clearLabels();
        setLabel(6, "Cancel", "", true);
        setLabel(7, "Yes", "", true);
        display.setControlColors(SI->label[6], display.getTheme().highlightedButtonLabelsColors);
        display.setControlColors(SI->label[7], display.getTheme().highlightedButtonLabelsColors);
        display.setAddControlStyle(SI->label[6], controlStyleBackground);
        display.setAddControlStyle(SI->label[7], controlStyleBackground);
        display.refreshControl(SI->label[6]);
        display.refreshControl(SI->label[7]);
        display.synchronizeRefresh();

        SI->ip->clearButtonsRange(interfaceButton0, interfaceButton7);
        SI->ip->setButtonObj(interfaceButton6, buttonPress, functOverwriteNo);
        SI->ip->setButtonObj(interfaceButton7, buttonPress, functOverwriteYes);
        SI->ip->blockAllInputsExcept(interfaceButton6, interfaceButton7);
        return;
    }
    startSaving();
}

// The name to start with: the .pti selected in the list (to replace it), else the instrument's
void functBrowserSave()
{
    char name[SAMPLE_NAME_SIZE + 1];
    polystrncpy(name, mtProject.instrument[saveSlot].sample.file_name, sizeof(name));
    const int pos = browserData.browserListPosition;
    if (pos < browserData.browserListLength && !browser.isFolder(pos) &&
        !(pos == 0 && !browser.isRoot()) && hasFileExtension(browserData.browserList[pos], "pti"))
    {
        polystrncpy(name, browserData.browserList[pos], sizeof(name));
        const size_t len = strlen(browserData.browserList[pos]);
        if (len >= 4 && len - 4 < sizeof(name))
        {
            name[len - 4] = 0;
        }
    }
    showKeyboard(name, SAMPLE_NAME_SIZE, "Save", functSaveConfirm);
}

void functBrowserCancel()
{
    SI->pnemSaveStop();
}

} // namespace

static_assert(sizeof(kInstrumentExtension) == 5, "\".pti\"");

//------------------------------------------------------------------------------------------------

void cSampleImporter::pnemShowRow()
{
    pnemLabelDataMore.dotIndicator.count = 2;
    pnemLabelDataMore.dotIndicator.current = pnemMorePage ? 1 : 0;
    display.setControlData(label[5], &pnemLabelDataMore);
    display.setAddControlStyle(label[5], controlStyleSpecialFx2);
    display.setControlColors(label[5], display.getTheme().moreButtonColors);
    display.refreshControl(label[5]);

    if (!pnemMorePage)
    {
        return;
    }
    display.setControlText(label[0], "Delete");
    display.setControlText2(label[0], "unused");
    display.setControlText(label[1], "Save");
    display.setControlText2(label[1], "instrument");
    for (uint8_t i = 2; i < 5; i++)
    {
        display.setControlText(label[i], "");
        display.setControlText2(label[i], "");
    }
    for (uint8_t i = 0; i < 2; i++)
    {
        display.setControlValue(label[i], 1); // two lines
        display.setControlStyle2(label[i], controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
    }
    pnemRow2Colors();
}

void cSampleImporter::pnemRow2Colors()
{
    // As Delete unused and Rename were: with the instrument list selected (Save: a sample in the slot)
    const uint32_t* active = display.getTheme().activeButtonLabelsColors;
    const uint32_t* inactive = display.getTheme().inactiveButtonLabelsColors;
    display.setControlColors(label[0], selectedPlace == 1 ? active : inactive);
    display.setControlColors(label[1],
                             selectedPlace == 1 && mtProject.instrument[selectedSlot].isActive == 1
                               ? active
                               : inactive);
    for (uint8_t i = 0; i < 5; i++)
    {
        display.refreshControl(label[i]);
    }
}

void cSampleImporter::pnemSaveInstrument()
{
    if (selectedPlace != 1 || mtProject.instrument[selectedSlot].isActive != 1)
    {
        return;
    }
    stopPlayingAll();
    engine.getSdPlayer()->stop(); // the folder list goes in the scratch memory the SD preview uses

    saveSlot = selectedSlot;
    placeBefore = selectedPlace;

    browserData.mode = kInstrumentBrowser; // folders, .pti and .wav
    browserData.selectionEnabled = false;
    browserData.maxFileNameLen = kMaxFileNameLen;
    browserData.maxBrowserListLen = kMaxFilesListLength;
    browserData.fileNamesBufferSize = kMaxFilesListLength * kMaxFileNameLen;
    browserData.fileNamesBuffer = reinterpret_cast<char*>(externalRamScratchBuff);
    browserData.defaultPath = "";
    browserData.selectionList = nullptr;
    browserData.browserListPosition = 0;
    if (lastPath[0] != 0 && !pnemFileSystem()->exists(lastPath))
    {
        lastPath[0] = 0;
    }
    polystrncpy(browserData.currentPath, lastPath, sizeof(browserData.currentPath));
    if (browserData.browserList == nullptr)
    {
        browserData.browserList = new char*[browserData.maxBrowserListLen];
    }
    browser.start(pnemFileSystem(), &browserData);
    browsing = true;

    showBrowser();
}

void cSampleImporter::pnemSaveCleanup()
{
    if (browserData.browserList != nullptr)
    {
        delete[] browserData.browserList;
        browserData.browserList = nullptr;
    }
    browsing = false;
}

void cSampleImporter::pnemSaveStop()
{
    const bool wasBrowsing = browsing;
    pnemSaveCleanup();
    keyboardManager.deactivateKeyboard();
    mtPopups.hideInfoPopup();
    if (wasBrowsing)
    {
        selectedPlace = placeBefore;
    }

    // The Sample Loader's own lists and screen again
    showFilesTree();
    setSelect();
    listInstrumentSlots();
    showInstrumentsList();
    setDefaultScreenFunct();
    showDefaultScreen();
    handleMemoryBar();
}

bool cSampleImporter::pnemSaveStatus(uint8_t status)
{
    if (status != fmSaveInstrumentEnd && status != fmSaveInstrumentError)
    {
        return false;
    }
    newFileManager.clearStatus();
    ip->unblockAllInputs();
    pnemSaveStop();
    if (status == fmSaveInstrumentEnd)
    {
        mtPopups.show(1, "Instrument saved.");
        // The folder on the left may be the one it went to
        if (newFileManager.browseSdCard(nullptr))
        {
            ip->blockAllInputs();
        }
    }
    else
    {
        mtPopups.show(1, "Saving the instrument failed.");
    }
    return true;
}
