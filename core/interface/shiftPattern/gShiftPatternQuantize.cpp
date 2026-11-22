// Shift + Pattern -> Quantize category: controls, drawing and editing. Compiled only with
// -DMOD_SHIFT_PATTERN_QUANTIZE (which requires -DMOD_PATTERN_SETTINGS).
#ifdef MOD_SHIFT_PATTERN_QUANTIZE

#ifndef MOD_PATTERN_SETTINGS
#error "MOD_SHIFT_PATTERN_QUANTIZE requires MOD_PATTERN_SETTINGS"
#endif

#include "shiftPattern/shiftPattern.h"

#include <stdio.h>

#include "quantizeSettings.h"

namespace
{
const char* const onOffNames[2] = { "On", "Off" };

// Display order is the reverse of quantizeGrid::Division: finest to coarsest.
const char* const divisionNames[quantizeGrid::divisionCount] = { "1/16", "1/8T", "1/16D", "1/8",
                                                                 "1/4T", "1/8D", "1/4", "1/4D" };

uint8_t toggleDisplayIndex(bool enabled)
{
    return enabled ? 0 : 1;
}

uint8_t divisionDisplayIndex(uint8_t division)
{
    return quantizeGrid::divisionCount - 1 - division;
}

// Bottom-row names. The full control names (Step Quantize, Quantize Division, Quarter Note Length) do not fit
// the 100 px column, so they are shortened here.
const char* const columnNames[8] = { "Category", "Note Off", "Quantize",  "Step Quant",
                                     "Division", "Strength", "Quarter Len", "Bypass" };

constexpr uint8_t kToggleNoteOff = 0;
constexpr uint8_t kToggleQuantize = 1;
constexpr uint8_t kToggleStepQuantize = 2;
constexpr uint8_t kToggleBypass = 3;

uint16_t columnX(uint8_t column)
{
    return (800 / 8) * column + 2;
}

bool toggleValue(uint8_t index)
{
    const quantizeSettings::Values& v = quantizeSettings::values;
    switch (index)
    {
        case kToggleNoteOff: return v.noteOff;
        case kToggleQuantize: return v.quantize;
        case kToggleStepQuantize: return v.stepQuantize;
        default: return v.bypass;
    }
}

uint8_t toggleColumn(uint8_t index)
{
    switch (index)
    {
        case kToggleNoteOff: return cShiftPattern::kQNoteOff;
        case kToggleQuantize: return cShiftPattern::kQQuantize;
        case kToggleStepQuantize: return cShiftPattern::kQStepQuantize;
        default: return cShiftPattern::kQBypass;
    }
}

int16_t barQuarter(uint8_t quarter)
{
    return ((quarter - quantizeGrid::kQuarterMin) * 100) /
           (quantizeGrid::kQuarterMax - quantizeGrid::kQuarterMin);
}
} // namespace

void cShiftPattern::initQuantizeControls()
{
    strControlProperties prop;
    prop.colors = nullptr;
    prop.y = 29;
    prop.w = 800 / 8 - 3;
    prop.h = 394;
    prop.style = controlStyleBackground |
                 (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);

    for (uint8_t i = 0; i < 4; i++)
    {
        qToggleList[i].linesCount = 2;
        qToggleList[i].start = 0;
        qToggleList[i].length = 2;
        qToggleList[i].data = (char**)onOffNames;
        prop.x = columnX(toggleColumn(i));
        prop.data = &qToggleList[i];
        qToggle[i] = display.createControl<cList>(&prop);
    }

    qDivisionList.linesCount = quantizeGrid::divisionCount;
    qDivisionList.start = divisionDisplayIndex(quantizeGrid::kDefaultDivision);
    qDivisionList.length = quantizeGrid::divisionCount;
    qDivisionList.data = (char**)divisionNames;
    prop.x = columnX(kQDivision);
    prop.data = &qDivisionList;
    qDivision = display.createControl<cList>(&prop);

    strControlProperties propBar;
    propBar.y = 29;
    propBar.w = 800 / 8 - 3;
    propBar.h = 394;
    propBar.colors = nullptr;
    propBar.style = controlStyleShow | controlStyleValue_0_100 | controlStyleBackground |
                    (display.getTheme().barShadow > 0 ? controlStyleGradient : 0) |
                    (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    propBar.x = columnX(kQStrength);
    qBar[0] = display.createControl<cBar>(&propBar);
    propBar.x = columnX(kQQuarter);
    qBar[1] = display.createControl<cBar>(&propBar);
}

void cShiftPattern::hideQuantizeControls()
{
    for (uint8_t i = 0; i < 4; i++)
    {
        display.setControlHide(qToggle[i]);
    }
    display.setControlHide(qDivision);
    display.setControlHide(qBar[0]);
    display.setControlHide(qBar[1]);
}

bool cShiftPattern::isQuantizeColumnEditable(uint8_t column) const
{
    if (!quantizeSettings::values.bypass)
    {
        return true;
    }
    // Bypass dims exactly Step Quantize, Division, Strength and Quarter Note Length.
    return !(column == kQStepQuantize || column == kQDivision || column == kQStrength || column == kQQuarter);
}

// Draws every Quantize control from the stored values; Bypass dims its four controls.
void cShiftPattern::refreshQuantizeValues()
{
    const quantizeSettings::Values& v = quantizeSettings::values;
    const Theme& theme = display.getTheme();

    snprintf(qStrengthText, sizeof(qStrengthText), "%d", v.strength);
    snprintf(qQuarterText, sizeof(qQuarterText), "%d", v.quarter);

    for (uint8_t i = 0; i < 4; i++)
    {
        const uint8_t col = toggleColumn(i);
        const bool active = isQuantizeColumnEditable(col);
        qToggleList[i].start = toggleDisplayIndex(toggleValue(i));
        display.setControlData(qToggle[i], &qToggleList[i]);
        display.setControlColors(qToggle[i], active ? theme.activeListColors : theme.inactiveListColors);
        display.setControlValue(qToggle[i], qToggleList[i].start);
        display.setControlShow(qToggle[i]);
        display.refreshControl(qToggle[i]);
    }

    {
        const bool active = isQuantizeColumnEditable(kQDivision);
        qDivisionList.start = divisionDisplayIndex(v.division);
        display.setControlData(qDivision, &qDivisionList);
        display.setControlColors(qDivision, active ? theme.activeListColors : theme.inactiveListColors);
        display.setControlValue(qDivision, divisionDisplayIndex(v.division));
        display.setControlShow(qDivision);
        display.refreshControl(qDivision);
    }

    display.setControlColors(qBar[0], isQuantizeColumnEditable(kQStrength) ? theme.activeBarColors
                                                                           : theme.inactiveBarColors);
    display.setControlValue(qBar[0], v.strength);
    display.setControlShow(qBar[0]);
    display.refreshControl(qBar[0]);

    display.setControlColors(qBar[1], isQuantizeColumnEditable(kQQuarter) ? theme.activeBarColors
                                                                          : theme.inactiveBarColors);
    display.setControlValue(qBar[1], barQuarter(v.quarter));
    display.setControlShow(qBar[1]);
    display.refreshControl(qBar[1]);

    // bottom labels: name + stored value; dimmed like an empty column when Bypass disables the control
    const char* values[8] = { nullptr,
                              onOffNames[toggleDisplayIndex(v.noteOff)],
                              onOffNames[toggleDisplayIndex(v.quantize)],
                              onOffNames[toggleDisplayIndex(v.stepQuantize)],
                              divisionNames[divisionDisplayIndex(v.division)],
                              qStrengthText,
                              qQuarterText,
                              onOffNames[toggleDisplayIndex(v.bypass)] };
    for (uint8_t col = kQNoteOff; col <= kQBypass; col++)
    {
        display.setControlText(label[col], columnNames[col]);
        display.setControlText2(label[col], values[col]);
        const bool active = isQuantizeColumnEditable(col);
        display.setControlColors(label[col],
                                 active ? theme.activeLabelsColors : theme.inactiveLabelsColors);
        display.setControlValue(label[col], active ? 1 : 0);
        display.setControlShow(label[col]);
        display.refreshControl(label[col]);
    }

    display.synchronizeRefresh();
}

void cShiftPattern::showQuantizeScreen()
{
    refreshQuantizeValues();
}

void cShiftPattern::changeQuantizeValue(int16_t value)
{
    if (!isQuantizeColumnEditable(selectedPlace))
    {
        return;
    }

    quantizeSettings::Values& v = quantizeSettings::values;
    switch (selectedPlace)
    {
        case kQNoteOff: if (value != 0) v.noteOff = value < 0; break;
        case kQQuantize: if (value != 0) v.quantize = value < 0; break;
        case kQStepQuantize: if (value != 0) v.stepQuantize = value < 0; break;
        case kQBypass: if (value != 0) v.bypass = value < 0; break;
        case kQDivision:
        {
            const int next = clamp<int>((int)v.division - value, 0, quantizeGrid::divisionCount - 1);
            v.division = (uint8_t)next;
            break;
        }
        case kQStrength:
        {
            v.strength = (uint8_t)clamp<int>((int)v.strength + value, 0, 100);
            break;
        }
        case kQQuarter:
        {
            v.quarter = (uint8_t)clamp<int>((int)v.quarter + value, quantizeGrid::kQuarterMin,
                                            quantizeGrid::kQuarterMax);
            break;
        }
        default: return;
    }

    refreshQuantizeValues();
}

void cShiftPattern::cycleQuantizeValue()
{
    if (!isQuantizeColumnEditable(selectedPlace))
    {
        return;
    }

    quantizeSettings::Values& v = quantizeSettings::values;
    switch (selectedPlace)
    {
        case kQNoteOff: v.noteOff = !v.noteOff; break;
        case kQQuantize: v.quantize = !v.quantize; break;
        case kQStepQuantize: v.stepQuantize = !v.stepQuantize; break;
        case kQBypass: v.bypass = !v.bypass; break;
        case kQDivision:
            v.division = (v.division == 0) ? quantizeGrid::divisionCount - 1 : v.division - 1;
            break;
        case kQQuarter:
            v.quarter = (v.quarter >= quantizeGrid::kQuarterMax) ? quantizeGrid::kQuarterMin : v.quarter + 1;
            break;
        default: return;
    }

    refreshQuantizeValues();
}

#endif // MOD_SHIFT_PATTERN_QUANTIZE
