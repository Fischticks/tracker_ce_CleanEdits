#ifndef INTERFACE_SHIFTPATTERN_H_
#define INTERFACE_SHIFTPATTERN_H_

// Shift + Pattern screen (custom). Category based, laid out like the Sample Recorder config screen:
// column 0 selects the category, the other seven columns operate the selected category.
// Everything in this module is compiled only with -DMOD_PATTERN_SETTINGS.
#ifdef MOD_PATTERN_SETTINGS

#include "core/modulesBase.h"

class cShiftPattern : public ModuleBase
{
  public:
    virtual void update();
    virtual void start(uint32_t options);
    virtual void stop();

    virtual void initDisplayControls();
    virtual void destroyDisplayControls() {}

    cShiftPattern() {}
    ~cShiftPattern() {}

    // Categories, in selector order. Quantize (MOD_SHIFT_PATTERN_QUANTIZE) takes index 0 and Pattern Settings
    // becomes the second category when it is compiled in.
    enum Category : uint8_t
    {
#ifdef MOD_SHIFT_PATTERN_QUANTIZE
        categoryQuantize = 0,
        categoryPatternSettings,
#else
        categoryPatternSettings = 0,
#endif
        categoryCount
    };

    // Columns of the Pattern Settings category (column 0 is the category selector).
    static constexpr uint8_t kColCategory = 0;
    static constexpr uint8_t kColLength = 1;
    static constexpr uint8_t kColDivider = 2;
    static constexpr uint8_t kColBpm = 7;

    // Reads the underlying settings and redraws the values that changed (all of them when force is set).
    void refreshValues(bool force);
    void showCategory();
    void showDefaultScreen();
    void activateFrame();

    void changeCategory(int16_t value, bool rollOver);
    void changeSelectedValue(int16_t value);
    void moveSelection(int8_t direction);
    void selectColumn(uint8_t column);
    bool isColumnSelectable(uint8_t column) const;

    // underlying settings (no copies are kept; these read and write the shared state)
    static uint8_t getPatternLength();             // user value 1..128
    static uint8_t getPatternDivider();            // user value 1..16
    void setPatternLength(uint8_t length);
    void setPatternDivider(uint8_t divider);
#ifdef MOD_PATTERN_BPM
    static uint16_t getBpm();                      // 0 = OFF, otherwise 10..400
    void setBpm(uint16_t bpm);
#endif

#ifdef MOD_SHIFT_PATTERN_QUANTIZE
    // Columns of the Quantize category (implemented in gShiftPatternQuantize.cpp).
    static constexpr uint8_t kQNoteOff = 1;
    static constexpr uint8_t kQQuantize = 2;
    static constexpr uint8_t kQStepQuantize = 3;
    static constexpr uint8_t kQDivision = 4;
    static constexpr uint8_t kQStrength = 5;
    static constexpr uint8_t kQQuarter = 6;
    static constexpr uint8_t kQBypass = 7;

    void initQuantizeControls();
    void showQuantizeScreen();   // shows and fills the Quantize controls (bottom labels included)
    void hideQuantizeControls();
    void refreshQuantizeValues();
    void changeQuantizeValue(int16_t value);
    bool isQuantizeColumnEditable(uint8_t column) const; // false for the controls dimmed by Bypass

    hControl qToggle[4]; // Note Off, Quantize, Step Quantize, Bypass
    hControl qDivision = -1;
    hControl qBar[2]; // Strength, Quarter Note Length
    strList qToggleList[4];
    strList qDivisionList;
    char qStrengthText[8];
    char qQuarterText[8];
#endif

    uint8_t selectedPlace = kColCategory;
    uint8_t category = 0; // kept between visits to the screen; the first category in the selector

    hControl label[8];
    hControl bgLabel = -1;
    hControl frameControl = -1;
    hControl categoryListControl = -1;
    hControl barControl[3];
    hControl blankSpaceCover = -1;

    strFrameData frameData;
    strList categoryList;

    char lengthText[8];
    char dividerText[8];
    char bpmText[8];

    // values last drawn, used by update() to follow changes made elsewhere (e.g. a pattern change while playing)
    uint8_t shownLength = 0;
    uint8_t shownDivider = 0;
    uint16_t shownBpm = 0xFFFF;
    bool shownValid = false;
};

extern cShiftPattern shiftPattern;

#endif // MOD_PATTERN_SETTINGS

#endif /* INTERFACE_SHIFTPATTERN_H_ */
