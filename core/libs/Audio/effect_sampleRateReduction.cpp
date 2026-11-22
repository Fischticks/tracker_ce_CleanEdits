#include "effect_sampleRateReduction.h"
#include <math.h>

float AudioSampleRateReduction::effectiveRateHz(uint8_t value)
{
    const float x = (float)value / 255.0f;
    return kMaxRateHz * expf(x * x * logf(kMinRateHz / kMaxRateHz));
}

AUDIO_CORE_FLASH void AudioSampleRateReduction::setReduction(uint8_t value)
{
    uint32_t increment = 0;
    if (value != 0)
    {
        const float ratio = effectiveRateHz(value) / kMaxRateHz;
        const float scaled = ratio * 4294967296.0f;
        increment = scaled >= 4294967040.0f ? 0xFFFFFF00u : (uint32_t)scaled;
        if (increment == 0)
            increment = 1;
    }

    if (phaseIncrement == 0 && increment != 0)
        captureNext = true;
    phaseIncrement = increment;
}

AUDIO_CORE_FLASH void AudioSampleRateReduction::reset()
{
    phase = 0;
    captureNext = true;
    held[0] = 0;
    held[1] = 0;
}

AUDIO_CORE_RAM void AudioSampleRateReduction::update()
{
    audio_block_t* input[2] = { receiveReadOnly(0), receiveReadOnly(1) };

    if (phaseIncrement == 0)
    {
        for (uint8_t channel = 0; channel < 2; channel++)
        {
            if (input[channel] != nullptr)
            {
                transmit(input[channel], channel);
                release(input[channel]);
            }
        }
        return;
    }

    const uint32_t increment = phaseIncrement;
    if (input[0] == nullptr && input[1] == nullptr && held[0] == 0 && held[1] == 0)
    {
        phase += increment * (uint32_t)AUDIO_BLOCK_SAMPLES;
        captureNext = false;
        return;
    }

    audio_block_t* output[2] = { nullptr, nullptr };
    for (uint8_t channel = 0; channel < 2; channel++)
    {
        if (input[channel] != nullptr)
            output[channel] = input[channel]->ref_count == 1 ? input[channel] : allocate();
        else if (held[channel] != 0)
            output[channel] = allocate();
    }

    uint32_t currentPhase = phase;
    bool firstCapture = captureNext;
    int16_t heldLeft = held[0];
    int16_t heldRight = held[1];

    for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; sample++)
    {
        const uint32_t previousPhase = currentPhase;
        currentPhase += increment;
        if (firstCapture || currentPhase < previousPhase)
        {
            heldLeft = input[0] != nullptr ? input[0]->data[sample] : 0;
            heldRight = input[1] != nullptr ? input[1]->data[sample] : 0;
            firstCapture = false;
        }

        if (output[0] != nullptr)
            output[0]->data[sample] = heldLeft;
        if (output[1] != nullptr)
            output[1]->data[sample] = heldRight;
    }

    phase = currentPhase;
    captureNext = firstCapture;
    held[0] = heldLeft;
    held[1] = heldRight;

    for (uint8_t channel = 0; channel < 2; channel++)
    {
        if (output[channel] != nullptr)
        {
            transmit(output[channel], channel);
            if (output[channel] != input[channel])
                release(output[channel]);
        }
        if (input[channel] != nullptr)
            release(input[channel]);
    }
}
