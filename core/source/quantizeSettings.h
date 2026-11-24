#ifndef QUANTIZE_SETTINGS_H_
#define QUANTIZE_SETTINGS_H_

// Runtime-only settings of Shift + Pattern -> Quantize (MOD_SHIFT_PATTERN_QUANTIZE). Not part of the project file:
// they are reset to the defaults below when a project is loaded or reloaded and, being RAM only, at power-up.
//
// Stored values are what the user selected. The effective values are what the recording and entry paths use:
// while Bypass is on, Step Quantize, Division, Strength and Quarter Note Length behave as their defaults without
// the stored values being touched. Note Off and Quantize are not affected by Bypass.

#ifdef MOD_SHIFT_PATTERN_QUANTIZE

#include <stdint.h>

#include "quantizeGrid.h"

namespace quantizeSettings
{
struct Values
{
    bool noteOff = true;       // write Note Off markers while recording
    bool quantize = false;     // quantize live-recorded notes (default Off)
    bool stepQuantize = false; // place manually entered notes on the grid and advance by the grid
    uint8_t division = quantizeGrid::kDefaultDivision;
    uint8_t strength = quantizeGrid::kDefaultStrength; // 0..100
    uint8_t quarter = quantizeGrid::kDefaultQuarter;   // 4..16
    bool bypass = false;
};

inline Values values;

inline void resetToDefaults()
{
    values = Values();
}

inline bool effectiveStepQuantize()
{
    return !values.bypass && values.stepQuantize;
}
inline uint8_t effectiveDivision()
{
    return values.bypass ? quantizeGrid::kDefaultDivision : values.division;
}
inline uint8_t effectiveStrength()
{
    return values.bypass ? quantizeGrid::kDefaultStrength : values.strength;
}
inline uint8_t effectiveQuarter()
{
    return values.bypass ? quantizeGrid::kDefaultQuarter : values.quarter;
}
inline int32_t effectiveInterval()
{
    return quantizeGrid::intervalTicks(effectiveDivision(), effectiveQuarter());
}

} // namespace quantizeSettings

#endif // MOD_SHIFT_PATTERN_QUANTIZE

#endif /* QUANTIZE_SETTINGS_H_ */
