#include "core/interfacePopups.h"
#include "fileManager.h"
#include "mtAudioEngine.h"
#include "samplePlayback.h"

static const uint16_t framesPlaces[7][4] = {
    { 0 + 1, 424, 800 / 8 - 2, 55 },
    { (800 / 8) * 1 + 1, 424, 800 / 8 - 2, 55 },
    { (800 / 8) * 2 + 1, 424, 800 / 8 - 2, 55 },
    { (800 / 8) * 3 + 1, 424, 800 / 8 - 2, 55 },
    { (800 / 8) * 4 + 1, 424, 800 / 8 - 2, 55 },
    { (800 / 8) * 5 + 1, 424, 800 / 8 - 2, 55 },
    { (800 / 8) * 6 + 1, 31, 800 / 4 - 3, 391 },
};

void cSamplePlayback::initDisplayControls()
{
    strControlProperties prop;
    frameData.placesCount = 7;
    frameData.startPlace = 0;
    frameData.places[0] = &framesPlaces[0][0];
    frameData.places[1] = &framesPlaces[1][0];
    frameData.places[2] = &framesPlaces[2][0];
    frameData.places[3] = &framesPlaces[3][0];
    frameData.places[4] = &framesPlaces[4][0];
    frameData.places[5] = &framesPlaces[5][0];
    frameData.places[6] = &framesPlaces[6][0];
    prop.style = 0;
    prop.value = 0;
    prop.colors = nullptr;
    prop.data = &frameData;
    frameControl = display.createControl<cFrame>(&prop);

    labelArrow.bitmaps[0].bitmapIndex = displayArrowU;
    labelArrow.bitmaps[0].xValue = (800 / 8) * 6 + (800 / 16) - 10;
    labelArrow.bitmaps[0].yValue = 450;
    labelArrow.bitmaps[1].bitmapIndex = displayArrowD;
    labelArrow.bitmaps[1].xValue = (800 / 8) * 7 + (800 / 16) + 10;
    labelArrow.bitmaps[1].yValue = 450;

    labelDataMore.dotIndicator.count = 2;
    labelDataMore.dotIndicator.current = 0;

    strControlProperties prop2;
    for (uint8_t i = 0; i < 8; i++)
    {
        prop2.value = 1;
        prop2.colors = display.getTheme().activeLabelsColors;
        prop2.style = (controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
        prop2.y = 424;
        prop2.h = 55;
        if (i == 6)
        {
            prop2.x = (800 / 8) * 6 + (800 / 8);
            prop2.w = 800 / 4 - 6;
        }
        else
        {
            prop2.x = (800 / 8) * i + (800 / 16);
            prop2.w = 800 / 8 - 6;
        }
        label[i] = display.createControl<cLabel>(&prop2);
    }

    prop2.text = nullptr;
    prop2.colors = nullptr;
    prop2.value = 255;
    prop2.style = controlStyleNoTransparency | controlStyleShow;
    prop2.x = 0;
    prop2.w = 800;
    prop2.y = 425;
    prop2.h = 54;
    bgLabel = display.createControl<cBgLabel>(&prop2);

    playModeList.linesCount = 8;
    playModeList.start = editorInstrument->playMode;
    playModeList.length = playModeCount;
    playModeList.data = playModeNames;
    prop.x = (800 / 8) * 6 + 1;
    prop.y = 29;
    prop.w = 800 / 4 - 3;
    prop.h = 394;
    prop.style = controlStyleBackground |
                 (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    prop.data = &playModeList;
    playModeListControl = display.createControl<cList>(&prop);

    // spectrum + points
    prop.style = 0;
    prop.x = 0;
    prop.y = 75 + ((300 - (MAX_16BIT / 300)) / 2);
    prop.w = 600;
    prop.h = MAX_16BIT / 300;
    progressCursor = display.createControl<cProgressCursor>(&prop);

    lineIndicatorSelection.start = 0;
    lineIndicatorSelection.end = 0;
    prop.data = &lineIndicatorSelection;
    prop.colors = display.getTheme().granularProgressColors;
    granularCursor = display.createControl<cLineIndicator>(&prop);

    prop.data = &points;
    prop.colors = nullptr;
    pointsControl = display.createControl<cPoints>(&prop);

    prop.data = &slicePoints;
    slicePointsControl = display.createControl<cSlicePoints>(&prop);

    spectrum[0].init();
    spectrum[1].init();

    prop.x = 0;
    prop.y = 75 + ((300 - ((MAX_16BIT / 300))) / 2);
    prop.w = 600;
    prop.h = (MAX_16BIT / 300) / 2;
    prop.data = &spectrum[0];
    spectrumControl[0] = display.createControl<cSpectrum>(&prop);

    prop.x = 0;
    prop.y = 75 + ((300 - ((MAX_16BIT / 300))) / 2) + (MAX_16BIT / 300) / 2;
    prop.w = 600;
    prop.h = (MAX_16BIT / 300) / 2;
    prop.data = &spectrum[1];
    spectrumControl[1] = display.createControl<cSpectrum>(&prop);

    prop.x = 0;
    prop.y = 400;
    prop.w = 600;
    prop.h = 2;
    prop.data = &wtPosition;
    wtPositionCursor = display.createControl<cWtProgress>(&prop);
}

void cSamplePlayback::showTitleBar()
{
    titleBar->show(true);
    titleBar->setTitle("Sample Playback");
    titleBar->showCurrentInstrumentOnRightLabel();
}

void cSamplePlayback::showDefaultScreen()
{
    showTitleBar();
    resizeWaveforms();
    showPlayModeList();

    display.setControlPosition(label[6], (800 / 8) * 6 + (800 / 8), 424);
    display.setControlSize(label[6], 800 / 4 - 6, 55);
    display.setControlData(label[6], &labelArrow);
    display.setAddControlStyle(label[6], controlStyleShowBitmap | controlStyleCenterY);
    display.setRemoveControlStyle(label[5], controlStyleSpecialFx2);

    display.setControlText(label[6], "Play Mode");
    display.setControlHide(label[7]);

    display.setControlColors(label[0], display.getTheme().activeLabelsColors);
    display.setControlColors(label[1], display.getTheme().activeLabelsColors);

    for (uint8_t i = 0; i < 7; i++)
    {
        display.setControlValue(label[i], 1);
        display.setControlStyle2(label[i],
                                 controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlText2(label[i], "");
        display.setControlShow(label[i]);
        display.refreshControl(label[i]);
    }
    display.setControlValue(label[6], 0);

    if (loadedInstrumentType == mtSampleTypeWaveFile)
    {
        display.setControlShow(progressCursor);
        display.refreshControl(progressCursor);

        if ((editorInstrument->playMode == playModeSlice) ||
            (editorInstrument->playMode == playModeBeatSlice))
        {
            display.setControlHide(pointsControl);
            display.setControlHide(granularCursor);
            display.setControlShow(slicePointsControl);
            display.refreshControl(slicePointsControl);

            display.setAddControlStyle(label[5], controlStyleSpecialFx2);
            display.setControlData(label[5], &labelDataMore);
            display.setControlColors(label[5], display.getTheme().moreButtonColors);

            // processWavetableCursor(editorInstrument->wavetableCurrentWindow);
            hideWavetablePositionCursor();

            if (!sliceScreenMode2)
            {
                labelDataMore.dotIndicator.current = 0;

                display.setControlText(label[0], "Slice");
                display.setControlText(label[1], "Adjust");
                display.setControlText(label[2], "Add");
                display.setControlText(label[3], "Remove");
                display.setControlText(label[4], "Zoom");
                display.setControlText(label[5], "More");

                display.setControlValue(label[2], 0);
                display.setControlValue(label[3], 0);

                showZoomValue();
                showSlicesAdjustValue();
                showSlicesSelectValue();
            }
            else
            {
                labelDataMore.dotIndicator.current = 1;

                display.setControlText(label[0], "Slice");
                display.setControlText(label[1], "Slice");
                display.setControlText(label[2], "No of Slices");
                display.setControlText(label[3], "Play Thru");
                display.setControlText(label[4], "");
                display.setControlText(label[5], "More");

                display.setControlText2(label[0], "Auto");
                display.setControlText2(label[1], "Evenly");

                display.setControlStyle2(
                  label[0], controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
                display.setControlStyle2(
                  label[1], controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
                display.setControlColors(label[0], display.getTheme().activeButtonLabelsColors);
                display.setControlColors(label[1], display.getTheme().activeButtonLabelsColors);

                showSlicesEqualNumberValue();
                showSlicesPlayThruValue();

                if (selectedPlace != 2 && selectedPlace != 3)
                {
                    selectedPlace = 2;
                }
            }
        }
        else if (editorInstrument->playMode == playModeGranular)
        {
            display.setControlText(label[0], "Preview");
            display.setControlText(label[1], "Position");
            display.setControlText(label[2], "Length");
            display.setControlText(label[3], "Shape");
            display.setControlText(label[4], "Loop");
            display.setControlText(label[5], "");

            display.setControlShow(granularCursor);
            display.refreshControl(granularCursor);

            display.setControlHide(pointsControl);
            display.refreshControl(pointsControl);

            display.setControlHide(slicePointsControl);
            display.refreshControl(slicePointsControl);

            showGranularPositionValue();
            showGrainLengthValue();
            showShapeText();
            showLoopTypeText();
            display.setControlText2(label[0], "-");
        }
        else
        {
            display.setControlHide(slicePointsControl);
            display.refreshControl(slicePointsControl);

            display.setControlHide(granularCursor);
            display.refreshControl(granularCursor);

            display.setControlText(label[0], "Preview");
            display.setControlText(label[1], "Start");
            display.setControlText(label[2], "");
            display.setControlText(label[3], "");
            display.setControlText(label[4], "End");
            display.setControlText(label[5], "Zoom");

            display.setControlShow(pointsControl);
            display.refreshControl(pointsControl);

            if ((editorInstrument->playMode == playModeSingleShot) ||
                (editorInstrument->playMode == playModeWavetable))
            {
                hideLoopPoints();
            }
            else
            {
                showLoopPoints();
            }

            showStartPointValue();
            showEndPointValue();
            showLoopPoint1Value();
            showLoopPoint2Value();
            showZoomValue();
            processWavetableCursor(editorInstrument->wavetableCurrentWindow);
            hideWavetablePositionCursor();

            display.setControlText2(label[0], "-");

            if (loopPagesActive())
            {
                showLoopPage(); // PNEM: Loop crossfade
            }
            else
            {
                // PNEM: Loop crossfade: Zoom's colours back after the loop pages' More
                display.setControlColors(label[5], display.getTheme().activeLabelsColors);
                if (selectedPlace == kPlaceCrossfade)
                {
                    selectedPlace = 6;
                }
            }
        }
    }
    else
    {
        display.setControlText(label[0], "Preview");
        display.setControlText(label[1], "Position");
        display.setControlText(label[2], "Window");
        display.setControlText(label[3], "");
        display.setControlText(label[4], "");
        display.setControlText(label[5], "");

        display.setControlValue(label[0], 0);

        // cursor
        display.setControlHide(progressCursor);
        display.refreshControl(progressCursor);

        display.setControlHide(granularCursor);
        display.refreshControl(granularCursor);
        // points
        display.setControlHide(pointsControl);
        display.refreshControl(pointsControl);

        display.setControlHide(slicePointsControl);
        display.refreshControl(slicePointsControl);

        showWavetablePosition();
        showWavetableWindowSize();
        refreshSpectrum = 1;

        processWavetableCursor(editorInstrument->wavetableCurrentWindow);
        showWavetablePositionCursor();
    }

    display.setControlValue(bgLabel, 127);
    display.setControlShow(bgLabel);
    display.refreshControl(bgLabel);

    mtPopups.hideInfoPopup();

    activateLabelsBorder();

    display.synchronizeRefresh();
}

//==============================================================================================================
void cSamplePlayback::hideUselessControls()
{
    display.setControlHide(pointsControl);
    display.setControlHide(frameControl);
    display.setControlHide(playModeListControl);
    display.setControlHide(slicePointsControl);
    display.setControlHide(granularCursor);
    display.setControlHide(progressCursor);
    display.setControlHide(wtPositionCursor);
    display.setControlHide(wtPositionCursor);
    display.setControlHide(spectrumControl[0]);
    display.setControlHide(spectrumControl[1]);
}

void cSamplePlayback::resizeWaveforms()
{
    if (newFileManager.isInstrumentStereo(mtProject.values.lastUsedInstrument) &&
        editorInstrument->isActive)
    {
        if ((editorInstrument->playMode == playModeSlice) ||
            (editorInstrument->playMode == playModeBeatSlice))
        {
            spectrum[0].pointsLimit = strTrackerSpectrum::k200;
            spectrum[1].pointsLimit = strTrackerSpectrum::k200;
        }
        else
        {
            spectrum[0].pointsLimit = strTrackerSpectrum::k300;
            spectrum[1].pointsLimit = strTrackerSpectrum::k300;
        }

        display.setControlSize(spectrumControl[0], 600, (MAX_16BIT / 300) / 2);

        display.setControlShow(spectrumControl[0]);
        display.setControlShow(spectrumControl[1]);

        display.refreshControl(spectrumControl[0]);
        display.refreshControl(spectrumControl[1]);
    }
    else
    {
        spectrum[0].pointsLimit = strTrackerSpectrum::k300;
        spectrum[1].pointsLimit = strTrackerSpectrum::k300;

        display.setControlSize(spectrumControl[0], 600, (MAX_16BIT / 300));

        display.setControlShow(spectrumControl[0]);
        display.setControlHide(spectrumControl[1]);

        display.refreshControl(spectrumControl[0]);
        display.refreshControl(spectrumControl[1]);
    }
}

//==============================================================================================================
void cSamplePlayback::activateLabelsBorder()
{
    const int8_t button = buttonOfPlace(selectedPlace);
    if (button < 0 || button > frameData.placesCount - 1)
        return;

    display.setControlValue(frameControl, button);
    display.setControlShow(frameControl);
    display.refreshControl(frameControl);
}

//==============================================================================================================
void cSamplePlayback::showWavetablePosition()
{
    if (refreshWavetablePosition)
        sprintf(wavetablePositionText, "%d", (int)currentEnvelopeWtPos);
    else
        sprintf(wavetablePositionText, "%d", (int)editorInstrument->wavetableCurrentWindow);

    display.setControlText2(label[1], wavetablePositionText);
    display.setControlShow(label[1]);
    display.refreshControl(label[1]);
}

void cSamplePlayback::showWavetableWindowSize()
{
    sprintf(wavetableWindowSizeText, "%d", editorInstrument->sample.wavetable_window_size);

    display.setControlText2(label[2], wavetableWindowSizeText);
    display.setControlShow(label[2]);
    display.refreshControl(label[2]);
}

void cSamplePlayback::showZoomValue()
{
    sprintf(zoomTextValue, "%.2f", zoom.zoomValue);

    if ((editorInstrument->playMode == playModeSlice) ||
        (editorInstrument->playMode == playModeBeatSlice))
    {
        display.setControlText2(label[4], zoomTextValue);
        display.setControlShow(label[4]);
        display.refreshControl(label[4]);
        return;
    }

    showPlaceValue(5, zoomTextValue);
}

void cSamplePlayback::showPlayModeList()
{
    playModeList.start = editorInstrument->playMode;
    playModeList.length = playModeCount;
    playModeList.linesCount = 9;
    playModeList.data = playModeNames;

    display.setControlData(playModeListControl, &playModeList);
    display.setControlShow(playModeListControl);
    display.refreshControl(playModeListControl);
}

void cSamplePlayback::showStartPointValue()
{

    if (playerEngine::isZeroInPointZone(editorInstrument->startPoint,
                                        editorInstrument->sample.length,
                                        editorInstrument->sample.address))
    {
        points.zerocrossed |= startZerocrossed;
    }
    else
    {
        points.zerocrossed &= (~startZerocrossed);
    }

    float recTimeValue = samplePosToTime(editorInstrument->sample.length);
    float localStartPoint = (recTimeValue * editorInstrument->startPoint) / MAX_16BIT;

    formatTimeSeconds(startPointValueText, sizeof(startPointValueText), localStartPoint);

    showPlaceValue(1, startPointValueText);
    display.refreshControl(pointsControl);
}

void cSamplePlayback::showEndPointValue()
{
    if (playerEngine::isZeroInPointZone(editorInstrument->endPoint,
                                        editorInstrument->sample.length,
                                        editorInstrument->sample.address))
    {
        points.zerocrossed |= endZerocrossed;
    }
    else
    {
        points.zerocrossed &= (~endZerocrossed);
    }

    float recTimeValue = samplePosToTime(editorInstrument->sample.length);
    float localEndPoint = (recTimeValue * editorInstrument->endPoint) / MAX_16BIT;
    formatTimeSeconds(endPointValueText, sizeof(endPointValueText), localEndPoint);

    showPlaceValue(4, endPointValueText);
    display.refreshControl(pointsControl);
}

void cSamplePlayback::showLoopPoint1Value()
{
    if (editorInstrument->playMode == playModeSingleShot)
    {
        return;
    }

    if (playerEngine::isZeroInPointZone(editorInstrument->loopPoint1,
                                        editorInstrument->sample.length,
                                        editorInstrument->sample.address))
    {
        points.zerocrossed |= loop1Zerocrossed;
    }
    else
    {
        points.zerocrossed &= (~loop1Zerocrossed);
    }

    float recTimeValue = samplePosToTime(editorInstrument->sample.length);
    float localLoopPoint1 = (recTimeValue * editorInstrument->loopPoint1) / MAX_16BIT;

    formatTimeSeconds(loopPoint1ValueText, sizeof(loopPoint1ValueText), localLoopPoint1);

    showPlaceValue(2, loopPoint1ValueText);
    display.refreshControl(pointsControl);
}

void cSamplePlayback::showLoopPoint2Value()
{
    if (editorInstrument->playMode == playModeSingleShot)
    {
        return;
    }

    if (playerEngine::isZeroInPointZone(editorInstrument->loopPoint2,
                                        editorInstrument->sample.length,
                                        editorInstrument->sample.address))
    {
        points.zerocrossed |= loop2Zerocrossed;
    }
    else
    {
        points.zerocrossed &= (~loop2Zerocrossed);
    }

    float recTimeValue = samplePosToTime(editorInstrument->sample.length);
    float localLoopPoint2 = (recTimeValue * editorInstrument->loopPoint2) / MAX_16BIT;

    formatTimeSeconds(loopPoint2ValueText, sizeof(loopPoint2ValueText), localLoopPoint2);

    showPlaceValue(3, loopPoint2ValueText);
    display.refreshControl(pointsControl);
}

void cSamplePlayback::showPreviewValue()
{
    float playTimeValue =
      playProgressValue * samplePosToTime(editorInstrument->sample.length) / MAX_16BIT;
    float localEndPoint =
      (editorInstrument->sample.length * editorInstrument->endPoint) / MAX_16BIT;
    if (playTimeValue >= (localEndPoint - 0.01))
        playTimeValue = localEndPoint;

    formatTimeSeconds(playTimeValueText, sizeof(playTimeValueText), playTimeValue);

    showPlaceValue(0, playTimeValueText);
}

void cSamplePlayback::hidePreviewValue()
{
    showPlaceValue(0, "-");
}

void cSamplePlayback::hideLoopPoints()
{
    display.setControlText2(label[2], "");
    display.setControlShow(label[2]);
    display.refreshControl(label[2]);

    display.setControlText2(label[3], "");
    display.setControlShow(label[3]);
    display.refreshControl(label[3]);
}

void cSamplePlayback::showLoopPoints()
{
    if (loopPagesActive())
    {
        showLoopPoint1Value();
        showLoopPoint2Value();
        return;
    }

    display.setControlText(label[2], "Loop Start");
    display.setControlShow(label[2]);
    display.refreshControl(label[2]);

    display.setControlText(label[3], "Loop End");
    display.setControlShow(label[3]);
    display.refreshControl(label[3]);

    showLoopPoint1Value();
    showLoopPoint2Value();
}

void cSamplePlayback::showWavetablePositionCursor()
{
    display.setControlData(wtPositionCursor, &wtPosition);
    display.setControlShow(wtPositionCursor);
    display.refreshControl(wtPositionCursor);
}

void cSamplePlayback::hideWavetablePositionCursor()
{
    display.setControlHide(wtPositionCursor);
    display.refreshControl(wtPositionCursor);
}

void cSamplePlayback::showAutoSlicePopup()
{
    mtPopups.showInfoPopup("This action will rewrite the current slices.",
                           "Do you want to continue?");

    for (uint8_t i = 0; i < 6; i++)
    {
        display.setControlHide(label[i]);
    }

    display.setControlShow(label[6]);
    display.setControlShow(label[7]);
    display.setControlText(label[6], "Cancel");
    display.setControlText(label[7], "Confirm");
    display.refreshControl(label[6]);
    display.refreshControl(label[7]);
    display.setControlValue(label[6], 0);
    display.setControlValue(label[7], 0);

    display.setControlPosition(label[6], (800 / 8) * 6 + (800 / 16), -1);
    display.setControlSize(label[6], 800 / 8 - 6, -1);
    display.setRemoveControlStyle(label[5], controlStyleSpecialFx2);
    display.setRemoveControlStyle(label[6], controlStyleShowBitmap);

    display.setControlValue(bgLabel, 255);
    display.setControlShow(bgLabel);
    display.refreshControl(bgLabel);

    display.setControlHide(frameControl);
    display.refreshControl(frameControl);

    display.synchronizeRefresh();
}

void cSamplePlayback::showSlicesSelectValue()
{
    if (editorInstrument->sliceNumber > 0)
    {
        sprintf(sliceSelectTextValue,
                "%d of %d",
                editorInstrument->selectedSlice + 1,
                editorInstrument->sliceNumber < 1 ? 1 : editorInstrument->sliceNumber);

        display.setControlText2(label[0], sliceSelectTextValue);
    }
    else
    {
        display.setControlText2(label[0], "-");
    }

    display.setControlShow(label[0]);
    display.refreshControl(label[0]);

    lightUpPadBoard();
}

void cSamplePlayback::showSlicesAdjustValue()
{
    if (editorInstrument->sliceNumber > 0)
    {
        float timeValue = samplePosToTime(editorInstrument->sample.length);
        uint16_t slice = (editorInstrument->sliceNumber == 0)
                           ? 0
                           : editorInstrument->slices[editorInstrument->selectedSlice];
        float currentSlice = (timeValue * slice) / MAX_16BIT;

        sprintf(sliceAdjustTextValue, "%.3f s", currentSlice);

        display.setControlText2(label[1], sliceAdjustTextValue);
        display.setControlShow(label[1]);
    }
    else
    {
        display.setControlText2(label[1], "-");
    }

    display.setControlShow(label[1]);
    display.refreshControl(label[1]);
}

void cSamplePlayback::showSlicesEqualNumberValue()
{
    sprintf(equalSliceNumberText, "%d", mtConfig.common.equalSliceNumber);
    display.setControlText2(label[2], equalSliceNumberText);
    display.setControlShow(label[2]);
    display.refreshControl(label[2]);
}

void cSamplePlayback::showSlicesPlayThruValue()
{
    display.setControlText2(label[3], editorInstrument->slicePlayThru ? "On" : "Off");
    display.setControlShow(label[3]);
    display.refreshControl(label[3]);
}

//==============================================================================================================
void cSamplePlayback::showGranularPositionValue()
{
    float localPosition =
      currentEnvelopeGranPos * samplePosToTime(editorInstrument->sample.length) / MAX_16BIT;

    sprintf(granularPositionTextValue, "%0.3f s", localPosition);

    uint32_t effectiveGrainLength =
      (currentEnvelopeGranLen > 0) ? currentEnvelopeGranLen
                                   : static_cast<uint32_t>(editorInstrument->granular.grainLength);
    uint16_t granularDisplayedLength =
      ((uint32_t)(effectiveGrainLength * 600)) /
      editorInstrument->sample.length;

    granularPositionInSpectrum = currentEnvelopeGranPos * (600.0 / MAX_16BIT);

    int upperZoneLength = granularDisplayedLength / 2 +
                          (granularDisplayedLength / 2 -
                           std::min((int)granularPositionInSpectrum, granularDisplayedLength / 2));
    int lowerZoneLength =
      granularDisplayedLength / 2 +
      (granularDisplayedLength / 2 -
       std::min((int)(600 - granularPositionInSpectrum), granularDisplayedLength / 2));

    lineIndicatorSelection.start = clamp(granularPositionInSpectrum - lowerZoneLength, 0, 600);
    lineIndicatorSelection.end = clamp(granularPositionInSpectrum + upperZoneLength, 0, 600);

    display.setControlValue(granularCursor, granularPositionInSpectrum);
    display.setControlShow(granularCursor);
    display.refreshControl(granularCursor);

    display.setControlText2(label[1], granularPositionTextValue);
    display.setControlShow(label[1]);
    display.refreshControl(label[1]);
}

void cSamplePlayback::showGrainLengthValue()
{
    grainLengthMs = editorInstrument->granular.grainLength / 44.1f;

    sprintf(grainLengthTextValue, "%0.1f ms", grainLengthMs);

    showGranularPositionValue();

    display.setControlText2(label[2], grainLengthTextValue);
    display.setControlShow(label[2]);
    display.refreshControl(label[2]);
}

void cSamplePlayback::showShapeText()
{
    switch (editorInstrument->granular.shape)
    {
        case granularShapeSquare: strcpy(shapeText, "Square"); break;
        case granularShapeTriangle: strcpy(shapeText, "Triangle"); break;
        case granularShapeGauss: strcpy(shapeText, "Gauss"); break;
        default: break;
    }

    display.setControlText2(label[3], shapeText);
    display.setControlShow(label[3]);
    display.refreshControl(label[3]);
}

void cSamplePlayback::showLoopTypeText()
{
    switch (editorInstrument->granular.type)
    {
        case granularLoopForward: strcpy(loopTypeText, "Forward"); break;
        case granularLoopBackward: strcpy(loopTypeText, "Backward"); break;
        case granularLoopPingPong: strcpy(loopTypeText, "PingPong"); break;
        default: break;
    }

    display.setControlText2(label[4], loopTypeText);
    display.setControlShow(label[4]);
    display.refreshControl(label[4]);
}

float cSamplePlayback::samplePosToTime(uint32_t pos)
{
    return pos / (44100.0 / (1 + editorInstrument->tuneResample / (-12)));
}
void cSamplePlayback::formatTimeSeconds(char* destBuffer, size_t destBufferSize, float value)
{
    int valueCharacters = destBufferSize - 2; // 's' and '/0'

    char* valueBuffer = new char[valueCharacters];

    snprintf(
      valueBuffer, valueCharacters, "%.3f", value); // keep constant number of characters for value

    snprintf(destBuffer, destBufferSize, "%ss", valueBuffer);

    delete[] valueBuffer;
}

//==============================================================================================================

//==============================================================================================================
// PNEM: Loop crossfade: the two button pages of Forward Loop and Backward Loop

bool cSamplePlayback::loopPagesActive() const
{
    return editorInstrument != nullptr && loadedInstrumentType == mtSampleTypeWaveFile &&
           (editorInstrument->playMode == playModeLoopForward ||
            editorInstrument->playMode == playModeLoopBackward);
}

// per page, the parameter (place) on buttons 0..4; -1 a blank button. Button 5 is More,
// buttons 6 and 7 the play mode list (place 6).
static const int8_t kLoopPagePlaces[2][5] = {
    { 0, 1, -1, 4, 5 },
    { 2, 3, cSamplePlayback::kPlaceCrossfade, -1, -1 },
};

static const char* const kLoopPageLabels[2][6] = {
    { "Preview", "Start", "", "End", "Zoom", "More" },
    { "Loop Start", "Loop End", "Crossfade", "", "", "More" },
};

int8_t cSamplePlayback::buttonOfPlace(uint8_t place) const
{
    if (!loopPagesActive())
    {
        return place == kPlaceCrossfade ? -1 : (int8_t)place;
    }
    if (place == 6)
    {
        return 6;
    }
    for (int8_t b = 0; b < 5; b++)
    {
        if (kLoopPagePlaces[loopScreenMode2][b] == place)
        {
            return b;
        }
    }
    return -1;
}

int8_t cSamplePlayback::placeOfButton(uint8_t button) const
{
    if (!loopPagesActive())
    {
        return (int8_t)button;
    }
    return button < 5 ? kLoopPagePlaces[loopScreenMode2][button] : -1;
}

hControl cSamplePlayback::labelOfPlace(uint8_t place) const
{
    const int8_t button = buttonOfPlace(place);
    return (button >= 0 && button < 6) ? label[button] : -1;
}

void cSamplePlayback::showPlaceValue(uint8_t place, const char* text)
{
    const hControl l = labelOfPlace(place);
    if (l < 0)
    {
        return;
    }
    display.setControlText2(l, text);
    display.setControlShow(l);
    display.refreshControl(l);
}

void cSamplePlayback::showCrossfadeValue()
{
    const uint16_t ms = editorInstrument->loopCrossfadeMs();
    snprintf(crossfadeValueText, sizeof(crossfadeValueText), "%u.%03us", ms / 1000u, ms % 1000u);
    showPlaceValue(kPlaceCrossfade, crossfadeValueText);
}

// After CE laid out the loop modes: the page that holds the selected parameter, its labels
// and values, and More with the page dots (as Slice's).
void cSamplePlayback::showLoopPage()
{
    if (selectedPlace == 2 || selectedPlace == 3 || selectedPlace == kPlaceCrossfade)
    {
        loopScreenMode2 = true;
    }
    else if (selectedPlace == 1 || selectedPlace == 4 || selectedPlace == 5)
    {
        loopScreenMode2 = false;
    }

    display.setAddControlStyle(label[5], controlStyleSpecialFx2);
    display.setControlData(label[5], &labelDataMore);
    display.setControlColors(label[5], display.getTheme().moreButtonColors);
    labelDataMore.dotIndicator.current = loopScreenMode2 ? 1 : 0;

    for (uint8_t i = 0; i < 6; i++)
    {
        display.setControlText(label[i], kLoopPageLabels[loopScreenMode2][i]);
        display.setControlText2(label[i], "");
        display.setControlShow(label[i]);
        display.refreshControl(label[i]);
    }

    if (!loopScreenMode2)
    {
        if (isPlayingSample)
            showPreviewValue();
        else
            hidePreviewValue();
        showStartPointValue();
        showEndPointValue();
        showZoomValue();
    }
    else
    {
        showLoopPoint1Value();
        showLoopPoint2Value();
        showCrossfadeValue();
    }
}
