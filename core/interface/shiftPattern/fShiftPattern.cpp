#ifdef MOD_PATTERN_SETTINGS

#include "shiftPattern/shiftPattern.h"

#include <math.h>

#include "core/interfaceMenuPopup.h"
#include "patternEditor/patternEditor.h"
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
#include "quantizeSettings.h"
#endif

#include "fileManager.h"
#include "mtSequencer.h"
#include "mtStructs.h"

cShiftPattern shiftPattern;
static cShiftPattern* SP = &shiftPattern;

static void functSwitchModule(uint8_t button);
static void functSelectButton(uint8_t button, uint8_t state);
static void functPlayAction();
static void functDeleteAction(uint8_t state);
static void functLeft();
static void functRight();
static void functUp();
static void functDown();
static void functEncoder(int16_t value);
static void functMenu(uint8_t state);

//==============================================================================================================
// Underlying settings. No state of its own is kept here.
//==============================================================================================================

// Pattern length: stored zero based per pattern (user value = stored + 1).
uint8_t cShiftPattern::getPatternLength()
{
    return sequencer.getPatternToUI()->track[0].length + 1;
}

void cShiftPattern::setPatternLength(uint8_t length)
{
    if (length < 1)
        length = 1;
    if (length > Sequencer::MAXSTEP + 1)
        length = Sequencer::MAXSTEP + 1;

    // The Pattern Editor's own setter: lengths of all eight sample tracks, cached length, cursor clamp and
    // the pattern's changed flag. Its display refresh is skipped because the editor is not on screen.
    patternEditor.applyActualPatternLength(length - 1);
}

// Pattern divider: project setting mtProject.values.patternDiv, stored zero based (user value = stored + 1).
uint8_t cShiftPattern::getPatternDivider()
{
    return mtProject.values.patternDiv + 1;
}

void cShiftPattern::setPatternDivider(uint8_t divider)
{
    if (divider < 1)
        divider = 1;
    if (divider > 16)
        divider = 16;

    mtProject.values.patternDiv = divider - 1;
    // The Pattern Editor derives trackerPattern.stepDevider from this value every time it is opened.
    newFileManager.setProjectStructChanged();
}

#ifdef MOD_PATTERN_BPM
uint16_t cShiftPattern::getBpm()
{
    return Sequencer::getPatternBpm(sequencer.getPatternToUI());
}

void cShiftPattern::setBpm(uint16_t bpm)
{
    Sequencer::setPatternBpm(sequencer.getPatternToUI(), bpm);
    newFileManager.setActualPatternStructChanged();
}
#endif

//==============================================================================================================

void cShiftPattern::start(uint32_t options)
{
    (void)options;

    moduleRefresh = 1;
    shownValid = false;

    if (category >= categoryCount)
    {
        category = categoryPatternSettings;
    }
    if (!isColumnSelectable(selectedPlace))
    {
        selectedPlace = kColCategory;
    }

    ip->setButtonObj(interfaceButtonPerformance, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonParams, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonFile, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonConfig, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonMaster, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSamplePlay, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleEdit, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleRec, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleLoad, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSong, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonPattern, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonPlay, buttonPress, functPlayAction);
    ip->setButtonObj(interfaceButtonDelete, functDeleteAction);

    ip->setButtonObj(interfaceButtonMiniMenu, functMenu);

    ip->setButtonObj(interfaceButtonLeft, buttonPress, functLeft);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functRight);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functUp);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDown);

    for (uint8_t i = 0; i < 8; i++)
    {
        ip->setButtonObj(interfaceButton0 + i, functSelectButton);
    }

    ip->setPotObj(interfacePot0, functEncoder);

    showDefaultScreen();
}

void cShiftPattern::stop()
{
    moduleRefresh = 0;
}

// Follows changes made elsewhere, e.g. the playing pattern changing while this screen is open.
void cShiftPattern::update()
{
    if (shownValid)
    {
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
        if (category != categoryPatternSettings)
        {
            return;
        }
#endif
        refreshValues(false);
    }
}

bool cShiftPattern::isColumnSelectable(uint8_t column) const
{
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
    if (category == categoryQuantize)
    {
        return column < 8; // all seven controls are selectable; Bypass only blocks editing
    }
#endif
    switch (column)
    {
        case kColCategory:
        case kColLength:
        case kColDivider: return true;
#ifdef MOD_PATTERN_BPM
        case kColBpm: return true;
#endif
        default: return false;
    }
}

void cShiftPattern::selectColumn(uint8_t column)
{
    if (!isColumnSelectable(column))
    {
        return;
    }

    selectedPlace = column;
    activateFrame();
    display.synchronizeRefresh();
}

void cShiftPattern::moveSelection(int8_t direction)
{
    int8_t column = selectedPlace;
    while (true)
    {
        column += direction;
        if (column < 0 || column > 7)
        {
            return; // no wrap around
        }
        if (isColumnSelectable(column))
        {
            selectColumn(column);
            return;
        }
    }
}

void cShiftPattern::changeCategory(int16_t value, bool rollOver)
{
    int16_t next = category + value;
    if (next < 0)
    {
        next = 0;
    }
    else if (next >= categoryCount)
    {
        next = rollOver ? 0 : categoryCount - 1;
    }

    category = next;
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
    // The categories have different controls: redraw the whole screen.
    showDefaultScreen();
#else
    showCategory();
    display.synchronizeRefresh();
#endif
}

void cShiftPattern::resetSelectedValue(uint8_t column)
{
    if (column == kColCategory)
    {
        return;
    }

    selectedPlace = column;
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
    if (category == categoryQuantize)
    {
        if (!isQuantizeColumnEditable(column))
        {
            return;
        }
        quantizeSettings::Values& v = quantizeSettings::values;
        const quantizeSettings::Values defaults;
        switch (column)
        {
            case kQNoteOff: v.noteOff = defaults.noteOff; break;
            case kQQuantize: v.quantize = defaults.quantize; break;
            case kQStepQuantize: v.stepQuantize = defaults.stepQuantize; break;
            case kQDivision: v.division = defaults.division; break;
            case kQStrength: v.strength = defaults.strength; break;
            case kQQuarter: v.quarter = defaults.quarter; break;
            case kQBypass: v.bypass = defaults.bypass; break;
            default: return;
        }
        refreshQuantizeValues();
        return;
    }
#endif
    switch (column)
    {
        case kColLength: setPatternLength(32); break;
        case kColDivider: setPatternDivider(4); break;
#ifdef MOD_PATTERN_BPM
        case kColBpm: setBpm(0); break; // OFF follows the Project BPM
#endif
        default: return;
    }
    refreshValues(false);
}

void cShiftPattern::changeSelectedValue(int16_t value)
{
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
    if (category == categoryQuantize)
    {
        changeQuantizeValue(value);
        return;
    }
#endif
    switch (selectedPlace)
    {
        case kColLength:
        {
            const int16_t next =
              clamp<int>((int)getPatternLength() + value, 1, Sequencer::MAXSTEP + 1);
            if (next != getPatternLength())
            {
                setPatternLength(next);
            }
            break;
        }
        case kColDivider:
        {
            const int16_t next = clamp<int>((int)getPatternDivider() + value, 1, 16);
            if (next != getPatternDivider())
            {
                setPatternDivider(next);
            }
            break;
        }
#ifdef MOD_PATTERN_BPM
        case kColBpm:
        {
            // OFF is its own state: OFF -> first step up starts at the Project BPM, and stepping down from the
            // minimum returns to OFF. Neither Tempo FX nor the Project BPM is touched.
            const uint16_t current = getBpm();
            int16_t next;
            if (current == 0)
            {
                if (value <= 0)
                {
                    return;
                }
                next = (int16_t)lroundf(mtProject.values.globalTempo) + value - 1;
            }
            else
            {
                next = (int16_t)current + value;
                if (next < Sequencer::kPatternBpmMin)
                {
                    next = 0;
                }
            }
            if (next != 0)
            {
                next = clamp<int>(next, Sequencer::kPatternBpmMin, Sequencer::kPatternBpmMax);
            }
            if (next != current)
            {
                setBpm(next);
            }
            break;
        }
#endif
        default: return;
    }

    refreshValues(false);
}

//==============================================================================================================
// Inputs
//==============================================================================================================

static void functPlayAction()
{
    if (sequencer.getSeqState() == Sequencer::SEQ_STATE_STOP)
    {
        sequencer.playPattern();
    }
    else
    {
        sequencer.stop();
        patternEditor.trackerPattern.playheadPosition = 0;
    }
}

static void functSwitchModule(uint8_t button)
{
    SP->eventFunct(eventSwitchModule, SP, &button, 0);
}

// Button N selects column N; pressing the category column again steps to the next category (wraps), as the
// Sample Recorder does for its source list.
static void functSelectButton(uint8_t button, uint8_t state)
{
    if (state != buttonPress)
    {
        return;
    }

#ifdef MOD_SHIFT_PATTERN_QUANTIZE
    if (SP->category == cShiftPattern::categoryQuantize && SP->selectedPlace == button &&
        button >= cShiftPattern::kQNoteOff && button <= cShiftPattern::kQBypass &&
        button != cShiftPattern::kQStrength && SP->isQuantizeColumnEditable(button))
    {
        SP->cycleQuantizeValue();
        return;
    }
#endif

    if (button == cShiftPattern::kColCategory && SP->selectedPlace == cShiftPattern::kColCategory)
    {
        SP->changeCategory(1, true);
    }

    SP->selectColumn(button);
}

static void functLeft()
{
    SP->moveSelection(-1);
}

static void functRight()
{
    SP->moveSelection(1);
}

// In the category column Up/Down walk the category list (Up = previous), elsewhere they step the value.
static void functUp()
{
    if (SP->selectedPlace == cShiftPattern::kColCategory)
    {
        SP->changeCategory(-1, false);
        return;
    }
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
    if (SP->category == cShiftPattern::categoryQuantize &&
        SP->selectedPlace >= cShiftPattern::kQNoteOff && SP->selectedPlace <= cShiftPattern::kQBypass &&
        SP->selectedPlace != cShiftPattern::kQStrength)
    {
        SP->changeSelectedValue(-1);
        return;
    }
#endif
    SP->changeSelectedValue(1);
}

static void functDown()
{
    if (SP->selectedPlace == cShiftPattern::kColCategory)
    {
        SP->changeCategory(1, false);
        return;
    }
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
    if (SP->category == cShiftPattern::categoryQuantize &&
        SP->selectedPlace >= cShiftPattern::kQNoteOff && SP->selectedPlace <= cShiftPattern::kQBypass &&
        SP->selectedPlace != cShiftPattern::kQStrength)
    {
        SP->changeSelectedValue(1);
        return;
    }
#endif
    SP->changeSelectedValue(-1);
}

static void functEncoder(int16_t value)
{
    if (SP->selectedPlace == cShiftPattern::kColCategory)
    {
        SP->changeCategory(interfaceGlobals.invertValForList ? -value : value, false);
    }
    else
    {
        SP->changeSelectedValue(value);
    }
}

static void functDeleteAction(uint8_t state)
{
    if (state != buttonPress)
    {
        return;
    }

    // Delete resets the control whose physical column button is being held.
    for (uint8_t column = 1; column < 8; column++)
    {
        if (mtInterface.getPlatform()->getButtons()->isButtonPressed(interfaceButton0 + column))
        {
            SP->resetSelectedValue(column);
            return;
        }
    }
}

static void functMenu(uint8_t state)
{
    if (state == buttonPress)
    {
        mtMenuPopup.showModulePopup(SP);
    }
    else if (state == buttonRelease)
    {
        mtMenuPopup.hideModulePopup(SP);
    }
}

#endif // MOD_PATTERN_SETTINGS
