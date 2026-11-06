
#include "mtStructs.h"

#include "fileManager.h"
#include "mtConfig.h"

#include "mtAudioEngine.h"
#include "mtSequencer.h"

#include "performanceFx.h"

#include "performanceEngine.h"

void saveProjectPerformanceValues()
{
    newFileManager.setProjectStructChanged();
}
void forceSaveProjectPerformanceValues()
{
    newFileManager.autosaveProjectStruct_ASAP();
}
// void saveConfigPerformanceValse() { saveConfig(); }

const uint8_t performanceDataSourceCount = 2;

struct PerformanceModeCC
{
    uint8_t isSelectedCC[12];
    int16_t ccValues[12];
} performanceModeCC;

strPerformDataSource performanceDataSource[performanceDataSourceCount] = {
    // global
    { mtConfig.values.perfTracksPatterns,
      mtConfig.values.perfFxPlaces,
      &mtConfig.values.perfFxValues[0][0],
      mtConfig.values.perfSelectedValues,
      mtConfig.values.perfTracksState,
      saveConfig,
      saveConfigAsap,
      performanceModeCC.ccValues,
      performanceModeCC.isSelectedCC },

    // project
    { mtProject.values.perfTracksPatterns,
      mtProject.values.perfFxPlaces,
      &mtProject.values.perfFxValues[0][0],
      mtProject.values.perfSelectedValues,
      mtProject.values.perfTracksState,
      saveProjectPerformanceValues,
      forceSaveProjectPerformanceValues,
      performanceModeCC.ccValues,
      performanceModeCC.isSelectedCC }
};

cPerformFx::cPerformFx(uint8_t fxId,
                       const char* fxName,
                       void (*fxSetFunct)(uint8_t, int16_t),
                       void (*fxClearFunct)(uint8_t),
                       const fxSetup* fxSetup)
  : id(fxId)
  , name(fxName)
  , setFunct(fxSetFunct)
  , clearFunct(fxClearFunct)
  , setup(fxSetup)
{
    performance.fxList[id] = this;
}

bool cPerformEngine::turnOff()
{
    bool wasPerformanceMode = sequencer.isPerformanceMode();
    if (wasPerformanceMode)
    {
        sequencer.exitPerformanceMode();
        engine.performanceModeEndAll();
    }

    for (uint8_t track = 0; track < kPerformanceTracks; track++)
    {
        tracksPerformanceState[track] = 0;
    }

    resetAllFx();

    return wasPerformanceMode;
}

void cPerformEngine::forceSaveConfig()
{
    if (sourceDataForceSaveFunct != nullptr)
    {
        sourceDataForceSaveFunct();
    }
}

const char* cPerformEngine::getSlotFxName(uint8_t slot)
{
    return fxList[slotToFx[slot]]->name;
}

// 0-3 zasiweca padem; 4 gasi wszystko - fx none
int8_t cPerformEngine::getSelectedRow(uint8_t slot)
{
    if (slot < 12 && slotToFx[slot] == 0)
        return -1;

    return slot < 12 ? slotToRow[slot] : 0;
}

uint8_t cPerformEngine::getTrackState(uint8_t track)
{
    return tracksPerformanceState[track];
}

uint8_t cPerformEngine::slotChanged(uint8_t slot)
{
    uint8_t flag = slotChangeFlag[slot];
    slotChangeFlag[slot] = 0;
    return flag > 0;
}

void cPerformEngine::setTrackState(uint8_t track, uint8_t state)
{
    tracksPerformanceState[track] = state;
}

// prezepisanie aktualnie zaznaczonych wartosci do chwilowych
void cPerformEngine::loadSelectedValuesToTempValues()
{
    for (uint8_t slot = 0; slot < 12; slot++)
    {
        if (slotToRow[slot] > 0)
        {
            slotsTempValues[slot] = *(slotRowToValue + (slot * 4) + slotToRow[slot]);
        }
        else
        {
            slotsTempValues[slot] = 0;
        }
    }
}

void cPerformEngine::toggleTrackState(uint8_t track)
{
    tracksPerformanceState[track] = !tracksPerformanceState[track];

    if (tracksPerformanceState[track] == 0)
    {
        for (uint8_t fx = 0; fx < fxCount; fx++)
        {
            resetFxOnTrack(fx, track);
        }
    }
    else
    {
        loadAllSlotsFxOnTrack(track);
    }
}

void cPerformEngine::fillSlotFxValue(uint8_t slot, uint8_t row, char* value)
{
    constexpr uint8_t CC_MODE_ROW = 4;
    int16_t fx_value = 0;

    // PNEM: Performance scenes: the column's values hold the scenes; it shows its labels
    if (isSceneSlot(slot))
    {
        const fxSetup* scene_setup = fxList[slotToFx[slot]]->setup;
        if (row < 4)
            strncpy(value, scene_setup->alternativeValuesLabels + scene_setup->alternativeValuesLabelsLength * row, 7);
        else
            strcpy(value, "---");
        return;
    }

    if (row == CC_MODE_ROW)
    {
        fx_value = slotToCCValue[slot];
        if (fxList[slotToFx[slot]]->setup->alternativeValuesLabels == nullptr)
        {
            if (fx_value == 0)
                strcpy(value, "---");
            else if (fx_value > 0)
                sprintf(value, "%d", fx_value);
            else
                sprintf(value, "%d", fx_value);
        }
        else
        {
            const fxSetup* temp_setup = fxList[slotToFx[slot]]->setup;
            if (fx_value > temp_setup->max)
                fx_value = temp_setup->max;
            strncpy(value,
                    temp_setup->alternativeValuesLabels +
                      (temp_setup->alternativeValuesLabelsLength * fx_value),
                    7);
        }

        return;
    }
    else if (slotToActiveCC[slot])
        return;

    if (row >= 0) // teraz wszystkie wiersze moga byc zmienialne; (row > 0) -> jesli 1 wiersz jest
                  // nie zmienny
    {
        fx_value =
          (row ==
           slotToRow[slot]) // ten zapis ustawia na start zaznaczone wartosci  fxow z tempValues
            ? slotsTempValues[slot] // zamiast poprawnych;
            : *(slotRowToValue + (slot * 4) +
                row); // wymaga wczesniejszego wypelnienia slotsTempValues poprawnymi wartosciami
    }

    if (fxList[slotToFx[slot]]->setup->alternativeValuesLabels == nullptr)
    {
        if (fx_value == 0)
            strcpy(value, "---");
        else
            sprintf(value, "%+d", fx_value);
    }
    else
    {
        const fxSetup* temp_setup = fxList[slotToFx[slot]]->setup;
        if (fx_value > temp_setup->max)
            fx_value = temp_setup->max;
        strncpy(value,
                temp_setup->alternativeValuesLabels +
                  (temp_setup->alternativeValuesLabelsLength * fx_value),
                7);
    }
}

void cPerformEngine::setSelectedRow(uint8_t slot, uint8_t row)
{
    if (slotToFx[slot] == 0)
        slotToRow[slot] = 0;
    else
        slotToRow[slot] = row;
}

void cPerformEngine::resetSelectedRows()
{
    for (int slotIdx = 0; slotIdx < 12; slotIdx++)
    {
        setSelectedRow(slotIdx, 0);
    }
}

void cPerformEngine::setSlotState(uint8_t slot, uint8_t state)
{
    slotsState[slot] = state;
}

void cPerformEngine::resetAllFx()
{
    for (uint8_t fx = 0; fx < fxCount; fx++)
    {
        if (fxList[fx]->setup->isGlobalFx)
        {
            resetGlobalFxValue(fx);
        }
        else
        {
            for (uint8_t track = 0; track < kPerformanceTracks; track++)
            {
                resetFxOnTrack(fx, track);
            }
        }
    }
}

void cPerformEngine::resetFxOnTrack(uint8_t fx, uint8_t track)
{
    if (fx == 0)
        return;

    if (fxList[fx]->setup->isGlobalFx)
    {
        return;
    }

    fxList[fx]->clearFunct(track);
}

void cPerformEngine::resetSlotFx(uint8_t slot)
{
    if (slot >= 12)
        return;

    uint8_t fxIdx = slotToFx[slot];
    if (fxIdx == 0)
        return;

    if (fxList[fxIdx]->setup->isGlobalFx)
    {
        resetGlobalFxValue(fxIdx);
    }
    else
    {
        for (uint8_t track = 0; track < kPerformanceTracks; track++)
        {
            // moze warto czyscic zapobiegawczo na wszystkich trakac? xxx
            if (tracksPerformanceState[track] == 1)
            {
                resetFxOnTrack(fxIdx, track);
            }
        }
    }
}

void cPerformEngine::loadAllSlotsFxOnTrack(uint8_t track)
{
    for (uint8_t slot = 0; slot < 12; slot++)
    {
        uint8_t temp_fx = slotToFx[slot];
        if (temp_fx == 0)
            continue;

        // zaladuj fx tylko na row > 0 i tlyko jesli wartosc != 0
        if (slotToRow[slot] > 0 && slotsTempValues[slot] != 0)
        {
            loadFxValuesOnTrack(slot, track);
        }
    }
}

void cPerformEngine::loadFxValuesOnTrack(uint8_t slot, uint8_t track)
{
    if (slot >= 12)
        return;

    uint8_t temp_fx = slotToFx[slot];
    if (temp_fx == 0)
        return;

    if (fxList[temp_fx]->setup->isGlobalFx)
    {
        return;
    }

    if (slotsTempValues[slot] == 0)
    {
        resetFxOnTrack(temp_fx, track);
        return;
    }

    fxList[temp_fx]->setFunct(track, slotsTempValues[slot]);
}

void cPerformEngine::loadGlobalFxValue(int fxIdx, int value)
{
    if (fxIdx > 0)
    {
        fxList[fxIdx]->setFunct(0, value);
    }
}

void cPerformEngine::resetGlobalFxValue(int fxIdx)
{
    if (fxIdx > 0)
    {
        fxList[fxIdx]->clearFunct(0);
    }
}

void cPerformEngine::loadSlotFxValues(uint8_t slot)
{
    // jezeli cos zdezaktywowalo slot to return
    if (slotsState[slot] == 0)
        return;

    resetOtherSlotsWithSameFx(slot);
    resetSlotsWithBlockedFx(slot);
    slotChangeFlag[slot] = 1;

    if (slotToRow[slot] == 0)
    {
        slotsTempValues[slot] = 0;
        resetSlotFx(slot);
        return;
    }

    slotsTempValues[slot] = *(slotRowToValue + (slot * 4) + slotToRow[slot]);

    int fxIdx = slotToFx[slot];
    if (fxIdx == 0)
    {
        return;
    }

    if (fxList[fxIdx]->setup->isGlobalFx)
    {
        int newValue = slotsTempValues[slot];
        loadGlobalFxValue(fxIdx, newValue);
    }
    else
    {
        for (uint8_t track = 0; track < kPerformanceTracks; track++)
        {
            if (tracksPerformanceState[track] == 1)
            {
                loadFxValuesOnTrack(slot, track);
            }
        }
    }
}

void cPerformEngine::loadCCValuesOnAffectedTracks(uint8_t slot)
{
    if (!slotToActiveCC[slot])
        return;
    if (isSceneSlot(slot)) // PNEM: Performance scenes don't follow MIDI CC
        return;

    resetOtherSlotsWithSameFx(slot);
    resetSlotsWithBlockedFx(slot);
    slotChangeFlag[slot] = 1;

    slotsTempValues[slot] = slotToCCValue[slot];

    int fxIdx = slotToFx[slot];
    if (fxIdx == 0)
    {
        return;
    }

    if (fxList[fxIdx]->setup->isGlobalFx)
    {
        int newValue = slotsTempValues[slot];
        loadGlobalFxValue(fxIdx, newValue);
    }
    else
    {
        for (uint8_t track = 0; track < kPerformanceTracks; track++)
        {
            if (tracksPerformanceState[track] == 1)
            {
                loadFxValuesOnTrack(slot, track);
            }
        }
    }
}

void cPerformEngine::resetOtherSlotsWithSameFx(uint8_t slot)
{
    for (uint8_t s = 0; s < 12; s++)
    {
        if (s == slot)
            continue;

        if (slotToFx[s] == slotToFx[slot])
        {
            slotsState[s] = 0;
            slotToRow[s] = 0;
            slotChangeFlag[s] = 1;
            slotsTempValues[s] = 0;
        }
    }
}

void cPerformEngine::resetSlotsWithBlockedFx(uint8_t slot)
{
    const fxSetup* temp_setup = fxList[slotToFx[slot]]->setup;
    if (temp_setup->blockedFx1 == 0 && temp_setup->blockedFx2 == 0)
        return;

    for (uint8_t s = 0; s < 12; s++)
    {
        if (s == slot)
            continue;

        if (slotToFx[s] == temp_setup->blockedFx1)
        {
            slotsState[s] = 0;
            slotToRow[s] = 0;
            slotChangeFlag[s] = 1;
            slotsTempValues[s] = 0;
        }

        if (slotToFx[s] == temp_setup->blockedFx2)
        {
            slotsState[s] = 0;
            slotToRow[s] = 0;
            slotChangeFlag[s] = 1;
            slotsTempValues[s] = 0;
        }
    }
}

void cPerformEngine::changeSlotFxValue(uint8_t slot, int16_t value)
{
    if (slot >= 12)
        return;
    if (!slotsState[slot])
        return;
    if (isSceneSlot(slot)) // PNEM: Performance scenes: no value to change
        return;
    // if(slotToRow[slot] == 0) return; // pierwszy tez modyfikujemy (od 1.1.17 ?)

    uint8_t fxIdx = slotToFx[slot];
    if (fxIdx == 0)
        return;

    // flaga zmiany wartosci slota
    slotChangeFlag[slot] = 1;

    // modyfikacja wartosci
    const fxSetup* temp_setup = fxList[slotToFx[slot]]->setup;
    if (slotsTempValues[slot] + value > temp_setup->max)
        slotsTempValues[slot] = temp_setup->max;
    else if (slotsTempValues[slot] + value < temp_setup->min)
        slotsTempValues[slot] = temp_setup->min;
    else
        slotsTempValues[slot] += value;

    if (editState && slotToRow[slot] > 0) // zapis jesli edit mode, tylko jesli nie pierwszy wiersz
    {
        *(slotRowToValue + (slot * 4) + slotToRow[slot]) = slotsTempValues[slot];
        if (sourceDataSaveFunct != nullptr)
            sourceDataSaveFunct();
    }

    if (fxList[fxIdx]->setup->isGlobalFx)
    {
        int newValue = slotsTempValues[slot];
        loadGlobalFxValue(fxIdx, newValue);
    }
    else
    {
        // przeladowanie efektu z nowa wartoscia
        for (uint8_t track = 0; track < kPerformanceTracks; track++)
        {
            if (tracksPerformanceState[track])
                loadFxValuesOnTrack(slot, track);
        }
    }
}

void cPerformEngine::changeSlotFx(uint8_t slot, int8_t change)
{
    uint8_t fx = slotToFx[slot];

    if (slotToRow[slot] > 0)
    {
        // konczy fx tylko jesli w tym slocie byla wybrana jakas wartosc (row > 0)
        if (fxList[fx]->setup->isGlobalFx)
        {
            resetGlobalFxValue(fx);
        }
        else
        {
            for (uint8_t track = 0; track < kPerformanceTracks; track++)
            {
                resetFxOnTrack(fx, track);
            }
        }
    }

    if (fx + change >= fxCount)
        fx = 0;
    else if (fx + change < 0)
        fx = fxCount - 1;
    else
        fx = fx + change;

    *(slotRowToValue + (4 * slot) + 0) = 0;
    *(slotRowToValue + (4 * slot) + 1) = 0;
    *(slotRowToValue + (4 * slot) + 2) = 0;
    *(slotRowToValue + (4 * slot) + 3) = 0;

    slotToRow[slot] = 0;
    slotsTempValues[slot] = 0;
    slotToFx[slot] = fx;

    // PNEM: Performance scenes: a new type starts with empty scenes
    clearSceneBits(slot);
    if (sceneEditSlot == slot)
        sceneEditSlot = -1;

    if (sourceDataSaveFunct != nullptr)
        sourceDataSaveFunct();
}

void cPerformEngine::swapSlotFx(uint8_t slotFrom, uint8_t slotTo)
{
    // PNEM: Performance scenes: a scene column takes its scenes along (by column number)
    SceneCopy scenesFrom, scenesTo;
    const bool sceneFrom = isSceneSlot(slotFrom);
    const bool sceneTo = isSceneSlot(slotTo);
    if (sceneFrom)
        readScenes(slotFrom, scenesFrom);
    if (sceneTo)
        readScenes(slotTo, scenesTo);
    sceneEditSlot = -1;

    int16_t temp_to1 = *(slotRowToValue + (4 * slotTo) + 0);
    int16_t temp_to2 = *(slotRowToValue + (4 * slotTo) + 1);
    int16_t temp_to3 = *(slotRowToValue + (4 * slotTo) + 2);
    int16_t temp_to4 = *(slotRowToValue + (4 * slotTo) + 3);

    *(slotRowToValue + (4 * slotTo) + 0) = *(slotRowToValue + (4 * slotFrom) + 0);
    *(slotRowToValue + (4 * slotTo) + 1) = *(slotRowToValue + (4 * slotFrom) + 1);
    *(slotRowToValue + (4 * slotTo) + 2) = *(slotRowToValue + (4 * slotFrom) + 2);
    *(slotRowToValue + (4 * slotTo) + 3) = *(slotRowToValue + (4 * slotFrom) + 3);

    *(slotRowToValue + (4 * slotFrom) + 0) = temp_to1;
    *(slotRowToValue + (4 * slotFrom) + 1) = temp_to2;
    *(slotRowToValue + (4 * slotFrom) + 2) = temp_to3;
    *(slotRowToValue + (4 * slotFrom) + 3) = temp_to4;

    uint8_t temp_to_fx = slotToFx[slotTo];
    slotToFx[slotTo] = slotToFx[slotFrom];
    slotToFx[slotFrom] = temp_to_fx;

    clearSceneBits(slotFrom);
    clearSceneBits(slotTo);
    if (sceneFrom)
        writeScenes(slotTo, scenesFrom);
    if (sceneTo)
        writeScenes(slotFrom, scenesTo);

    if (sourceDataSaveFunct != nullptr)
        sourceDataSaveFunct();
}

void cPerformEngine::copySlotFx(uint8_t slot)
{
    if (slot >= 12)
        return;

    isSlotCopied = true;
    slotCopied = slot;
}

void cPerformEngine::pasteSlotFx(uint8_t slot)
{
    if (slot >= 12 || slotCopied >= 12)
        return;

    if (!isSlotCopied)
        return;

    // PNEM: Performance scenes: a pasted scene column gets the scenes (by column number)
    SceneCopy scenes;
    const bool sceneCopied = isSceneSlot(slotCopied);
    if (sceneCopied)
        readScenes(slotCopied, scenes);
    if (sceneEditSlot == slot)
        sceneEditSlot = -1;

    *(slotRowToValue + (4 * slot) + 0) = *(slotRowToValue + (4 * slotCopied) + 0);
    *(slotRowToValue + (4 * slot) + 1) = *(slotRowToValue + (4 * slotCopied) + 1);
    *(slotRowToValue + (4 * slot) + 2) = *(slotRowToValue + (4 * slotCopied) + 2);
    *(slotRowToValue + (4 * slot) + 3) = *(slotRowToValue + (4 * slotCopied) + 3);

    slotToFx[slot] = slotToFx[slotCopied];
    slotToRow[slot] = 0;

    clearSceneBits(slot);
    if (sceneCopied)
        writeScenes(slot, scenes);

    if (sourceDataSaveFunct != nullptr)
        sourceDataSaveFunct();
}

// CC mode
void cPerformEngine::setCCValue(uint8_t value, uint8_t slot)
{
    if (isSceneSlot(slot)) // PNEM: Performance scenes don't follow MIDI CC
        return;
    constexpr uint8_t CC_VALUE_MIN = 0;
    constexpr uint8_t CC_VALUE_MAX = 127;

    const fxSetup* slotFxSetup = fxList[slotToFx[slot]]->setup;

    slotToCCValue[slot] =
      mapClamped<int>(value, CC_VALUE_MIN, CC_VALUE_MAX, slotFxSetup->min, slotFxSetup->max);
    setCCMode(slot);
}
void cPerformEngine::setCCMode(uint8_t slot)
{
    slotToActiveCC[slot] = 1;
}

void cPerformEngine::clearCC(uint8_t slot)
{
    slotToCCValue[slot] = 0;
    clearCCMode(slot);
}
void cPerformEngine::clearCCMode(uint8_t slot)
{
    slotToActiveCC[slot] = 0;
}

//##################################################################################################
// PNEM: Performance scenes

bool cPerformEngine::isSceneSlot(uint8_t slot) const
{
    return slot < 12 && slotToFx != nullptr && slotToFx[slot] == performanceSceneFxId;
}

// A column a scene sets: not the scene column itself, not empty, not another scene column.
bool cPerformEngine::isSceneEligible(uint8_t sceneSlot, uint8_t column) const
{
    return column < 12 && column != sceneSlot && slotToFx[column] != 0 && !isSceneSlot(column);
}

// Scene scene (0..2), column (0..11, not the scene column): 2 bits at 2 * (scene * 11 + j), j
// counting the other columns. Bits 0..63 are the scene column's four values, 64..65 trackState.
uint8_t cPerformEngine::scenePosition(uint8_t sceneSlot, uint8_t scene, uint8_t column) const
{
    if (column == sceneSlot || column >= 12 || scene >= 3 || slotExtraBits == nullptr)
        return 0;
    const uint32_t bit = 2u * (scene * 11u + (column < sceneSlot ? column : column - 1u));
    if (bit < 64)
    {
        const uint16_t word = (uint16_t) * (slotRowToValue + (sceneSlot * 4) + (bit / 16));
        return (word >> (bit % 16)) & 3;
    }
    return (slotExtraBits[sceneSlot / 4] >> ((sceneSlot % 4) * 2)) & 3;
}

void cPerformEngine::setScenePosition(uint8_t sceneSlot, uint8_t scene, uint8_t column, uint8_t position)
{
    if (column == sceneSlot || column >= 12 || scene >= 3 || slotExtraBits == nullptr)
        return;
    position &= 3;
    const uint32_t bit = 2u * (scene * 11u + (column < sceneSlot ? column : column - 1u));
    if (bit < 64)
    {
        int16_t* value = slotRowToValue + (sceneSlot * 4) + (bit / 16);
        uint16_t word = (uint16_t)*value;
        word = (uint16_t)((word & ~(3u << (bit % 16))) | ((uint32_t)position << (bit % 16)));
        *value = (int16_t)word;
        return;
    }
    const uint8_t shift = (sceneSlot % 4) * 2;
    slotExtraBits[sceneSlot / 4] =
      (uint8_t)((slotExtraBits[sceneSlot / 4] & ~(3u << shift)) | (position << shift));
}

void cPerformEngine::readScenes(uint8_t slot, SceneCopy& copy) const
{
    for (uint8_t scene = 0; scene < 3; scene++)
        for (uint8_t column = 0; column < 12; column++)
            copy.position[scene][column] = scenePosition(slot, scene, column);
}

void cPerformEngine::writeScenes(uint8_t slot, const SceneCopy& copy)
{
    for (uint8_t row = 0; row < 4; row++)
        *(slotRowToValue + (slot * 4) + row) = 0;
    clearSceneBits(slot);
    for (uint8_t scene = 0; scene < 3; scene++)
        for (uint8_t column = 0; column < 12; column++)
            setScenePosition(slot, scene, column, copy.position[scene][column]);
}

void cPerformEngine::clearSceneBits(uint8_t slot)
{
    if (slot >= 12 || slotExtraBits == nullptr)
        return;
    slotExtraBits[slot / 4] = (uint8_t)(slotExtraBits[slot / 4] & ~(3u << ((slot % 4) * 2)));
}

// A column to a row as its pad would put it (the row's effect on, or off at row 0).
void cPerformEngine::setSlotRowNow(uint8_t slot, uint8_t row)
{
    const uint8_t wasHeld = slotsState[slot];
    setSelectedRow(slot, row);
    slotsState[slot] = 1;
    loadSlotFxValues(slot);
    slotsState[slot] = wasHeld;
}

// Row 0 (Off) turns every other column off; 1..3 put them where the scene has them. Only
// columns that change are touched: first the ones going off, then the ones going on (a column
// going on clears the others of its type).
void cPerformEngine::applyScene(uint8_t sceneSlot, uint8_t row)
{
    for (uint8_t pass = 0; pass < 2; pass++)
    {
        for (uint8_t column = 0; column < 12; column++)
        {
            if (!isSceneEligible(sceneSlot, column))
                continue;
            const uint8_t position = row ? scenePosition(sceneSlot, row - 1, column) : 0;
            if (position == slotToRow[column] || (pass == 0) != (position == 0))
                continue;
            setSlotRowNow(column, position);
        }
    }
}

// Every other column's position into the scene.
void cPerformEngine::storeScene(uint8_t sceneSlot, uint8_t scene)
{
    for (uint8_t column = 0; column < 12; column++)
    {
        if (column == sceneSlot)
            continue;
        const uint8_t row = slotToRow[column];
        setScenePosition(sceneSlot, scene, column, isSceneEligible(sceneSlot, column) && row < 4 ? row : 0);
    }
    if (sourceDataSaveFunct != nullptr)
        sourceDataSaveFunct();
}

void cPerformEngine::scenePadPressed(uint8_t slot, uint8_t row)
{
    if (!isSceneSlot(slot) || row > 3)
        return;

    if (editState)
    {
        sceneEditSlot = row > 0 ? (int8_t)slot : -1;
        sceneEditIdx = row > 0 ? row - 1 : 0;
    }
    applyScene(slot, row);

    // Held: the encoder moves what the scene turned on (Off: every column at its top pad), as
    // CE moves a held pad's effect. Not in Rec, where the encoder would change the values.
    if (editState)
        return;
    for (uint8_t column = 0; column < 12; column++)
    {
        if (!isSceneEligible(slot, column))
            continue;
        const bool moves = row ? scenePosition(slot, row - 1, column) > 0 : slotToRow[column] == 0;
        if (moves)
        {
            sceneHeld[column] = true;
            slotsState[column] = 1;
        }
    }
}

void cPerformEngine::scenePadReleased(uint8_t slot)
{
    (void)slot;
    for (uint8_t column = 0; column < 12; column++)
    {
        if (!sceneHeld[column])
            continue;
        sceneHeld[column] = false;
        // back to the Rec value, or off, if the encoder moved it
        const int16_t recValue =
          slotToRow[column] > 0 ? *(slotRowToValue + (column * 4) + slotToRow[column]) : 0;
        if (slotsTempValues[column] != recValue)
        {
            slotsState[column] = 1;
            loadSlotFxValues(column);
        }
        slotsState[column] = 0;
    }
}

void cPerformEngine::otherPadPressed(uint8_t slot)
{
    if (!editState || sceneEditSlot < 0 || !isSceneSlot(sceneEditSlot) || slot == sceneEditSlot ||
        isSceneSlot(slot))
        return;
    storeScene(sceneEditSlot, sceneEditIdx);
}

void cPerformEngine::sceneEditBegin()
{
    sceneEditSlot = -1;
    for (uint8_t slot = 0; slot < 12; slot++)
    {
        if (isSceneSlot(slot) && slotToRow[slot] >= 1 && slotToRow[slot] <= 3)
        {
            sceneEditSlot = slot;
            sceneEditIdx = slotToRow[slot] - 1;
            return;
        }
    }
}

void cPerformEngine::sceneEditEnd()
{
    sceneEditSlot = -1;
}

uint8_t cPerformEngine::sceneEditNumber() const
{
    return (sceneEditSlot >= 0 && isSceneSlot(sceneEditSlot)) ? sceneEditIdx + 1 : 0;
}
