#ifdef MOD_PATTERN_SETTINGS

#include "shiftPattern/shiftPattern.h"

#include <stdio.h>

#include "mtSequencer.h"

namespace
{
constexpr uint8_t kLengthMax = Sequencer::MAXSTEP + 1; // 128 steps
constexpr uint8_t kDividerMax = 16;

// Same column frames as the Sample Recorder config screen.
uint16_t framesPlaces[8][4] = { { 0 + 1, 31, 800 / 8 - 3, 391 },
                                { (800 / 8) * 1 + 1, 31, 800 / 8 - 3, 391 },
                                { (800 / 8) * 2 + 1, 31, 800 / 8 - 3, 391 },
                                { (800 / 8) * 3 + 1, 31, 800 / 8 - 3, 391 },
                                { (800 / 8) * 4 + 1, 31, 800 / 8 - 3, 391 },
                                { (800 / 8) * 5 + 1, 31, 800 / 8 - 3, 391 },
                                { (800 / 8) * 6 + 1, 31, 800 / 8 - 3, 391 },
                                { (800 / 8) * 7 + 1, 31, 800 / 8 - 3, 391 } };

// Only Pattern Settings exists today; Quantize will be added in front of it.
const char* const categoryNames[cShiftPattern::categoryCount] = { "Pattern Settings" };

// bar position (0..100) of each value
int16_t barLength(uint8_t length)
{
    return ((length - 1) * 100) / (kLengthMax - 1);
}

int16_t barDivider(uint8_t divider)
{
    return ((divider - 1) * 100) / (kDividerMax - 1);
}

#ifdef MOD_PATTERN_BPM
int16_t barBpm(uint16_t bpm)
{
    if (bpm == 0)
    {
        return 0;
    }
    return 1 + ((bpm - Sequencer::kPatternBpmMin) * 99) /
                 (Sequencer::kPatternBpmMax - Sequencer::kPatternBpmMin);
}
#endif
} // namespace

void cShiftPattern::initDisplayControls()
{
    strControlProperties prop;
    for (uint8_t i = 0; i < 8; i++)
    {
        prop.value = 1;
        prop.colors = display.getTheme().activeLabelsColors;
        prop.text = (char*)"";
        prop.style = (controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
        prop.x = (800 / 8) * i + (800 / 16);
        prop.w = 800 / 8 - 6;
        prop.y = 424;
        prop.h = 55;
        label[i] = display.createControl<cLabel>(&prop);
    }

    // selection frame
    strControlProperties prop3;
    frameData.placesCount = 8;
    frameData.startPlace = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        frameData.places[i] = &framesPlaces[i][0];
    }
    prop3.style = 0;
    prop3.value = 0;
    prop3.data = &frameData;
    frameControl = display.createControl<cFrame>(&prop3);

    strControlProperties prop2;
    prop2.text = nullptr;
    prop2.colors = nullptr;
    prop2.value = 255;
    prop2.style = controlStyleNoTransparency | controlStyleShow;
    prop2.x = 0;
    prop2.w = 800;
    prop2.y = 425;
    prop2.h = 54;
    bgLabel = display.createControl<cBgLabel>(&prop2);

    // category selector (column 0)
    strControlProperties prop4;
    categoryList.linesCount = categoryCount;
    categoryList.start = category;
    categoryList.length = categoryCount;
    categoryList.data = (char**)categoryNames;
    prop4.x = 0 + 2;
    prop4.y = 29;
    prop4.w = 800 / 8 - 3;
    prop4.h = 394;
    prop4.data = &categoryList;
    prop4.style = controlStyleBackground |
                  (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    categoryListControl = display.createControl<cList>(&prop4);

    // value sliders
    strControlProperties prop5;
    prop5.x = (800 / 8) * kColLength + 2;
    prop5.y = 29;
    prop5.w = 800 / 8 - 3;
    prop5.h = 394;
    prop5.colors = nullptr;
    prop5.style = controlStyleShow | controlStyleValue_0_100 | controlStyleBackground |
                  (display.getTheme().barShadow > 0 ? controlStyleGradient : 0) |
                  (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    barControl[0] = display.createControl<cBar>(&prop5);
    prop5.x = (800 / 8) * kColDivider + 2;
    barControl[1] = display.createControl<cBar>(&prop5);
    prop5.x = (800 / 8) * kColBpm + 2;
    barControl[2] = display.createControl<cBar>(&prop5);

    // empty space of the columns kept free for future controls
    strControlProperties prop6;
    prop6.value = 0;
    prop6.data = nullptr;
    prop6.text = nullptr;
    prop6.colors = display.getTheme().blankSpaceCoverLabelColors;
    prop6.style = controlStyleBackground |
                  (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    prop6.x = (800 / 8) * 3 + 2;
#ifdef MOD_PATTERN_BPM
    prop6.w = (800 / 8) * 4 - 3; // columns 3..6
#else
    prop6.w = (800 / 8) * 5 - 3; // columns 3..7 (no Pattern BPM control in this build)
#endif
    prop6.y = 29;
    prop6.h = 394;
    blankSpaceCover = display.createControl<cLabel>(&prop6);
}

void cShiftPattern::showCategory()
{
    categoryList.start = category;
    categoryList.length = categoryCount;
    categoryList.linesCount = categoryCount;
    categoryList.data = (char**)categoryNames;

    display.setControlData(categoryListControl, &categoryList);
    display.setControlValue(categoryListControl, category);
    display.setControlShow(categoryListControl);
    display.refreshControl(categoryListControl);

    display.setControlText(label[kColCategory], "Category");
    display.setControlText2(label[kColCategory], categoryNames[category]);
    display.refreshControl(label[kColCategory]);
}

void cShiftPattern::activateFrame()
{
    display.setControlValue(frameControl, selectedPlace);
    display.setControlShow(frameControl);
    display.refreshControl(frameControl);
}

void cShiftPattern::refreshValues(bool force)
{
    const uint8_t length = getPatternLength();
    const uint8_t divider = getPatternDivider();

    if (force || !shownValid || length != shownLength)
    {
        shownLength = length;
        snprintf(lengthText, sizeof(lengthText), "%d", length);
        display.setControlText(label[kColLength], "Length");
        display.setControlText2(label[kColLength], lengthText);
        display.refreshControl(label[kColLength]);
        display.setControlValue(barControl[0], barLength(length));
        display.setControlShow(barControl[0]);
        display.refreshControl(barControl[0]);
    }

    if (force || !shownValid || divider != shownDivider)
    {
        shownDivider = divider;
        snprintf(dividerText, sizeof(dividerText), "%d", divider);
        display.setControlText(label[kColDivider], "Divider");
        display.setControlText2(label[kColDivider], dividerText);
        display.refreshControl(label[kColDivider]);
        display.setControlValue(barControl[1], barDivider(divider));
        display.setControlShow(barControl[1]);
        display.refreshControl(barControl[1]);
    }

#ifdef MOD_PATTERN_BPM
    const uint16_t bpm = getBpm();
    if (force || !shownValid || bpm != shownBpm)
    {
        shownBpm = bpm;
        if (bpm == 0)
        {
            snprintf(bpmText, sizeof(bpmText), "OFF");
        }
        else
        {
            snprintf(bpmText, sizeof(bpmText), "%d", bpm);
        }
        display.setControlText(label[kColBpm], "BPM");
        display.setControlText2(label[kColBpm], bpmText);
        display.refreshControl(label[kColBpm]);
        display.setControlValue(barControl[2], barBpm(bpm));
        display.setControlShow(barControl[2]);
        display.refreshControl(barControl[2]);
    }
#else
    display.setControlHide(barControl[2]);
#endif

    shownValid = true;
    display.synchronizeRefresh();
}

void cShiftPattern::showDefaultScreen()
{
    titleBar->show();
    titleBar->setTitle("Pattern Settings");

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlText(label[i], "");
        display.setControlText2(label[i], "");
        display.setControlStyle(label[i],
                                controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlColors(label[i], display.getTheme().activeLabelsColors);
        display.setControlValue(label[i], 1);
    }

    // columns 3..6 are intentionally empty (reserved for future controls); so is column 7 without Pattern BPM
    for (uint8_t i = 3; i < 8; i++)
    {
        if (!isColumnSelectable(i))
        {
            display.setControlValue(label[i], 0);
        }
    }

    display.setControlValue(bgLabel, 255);
    display.refreshControl(bgLabel);

    display.setControlShow(blankSpaceCover);
    display.refreshControl(blankSpaceCover);

    showCategory();
    refreshValues(true);
    activateFrame();

    display.synchronizeRefresh();
}

#endif // MOD_PATTERN_SETTINGS
