#include "filter_tone.h"
#include <math.h>

float AudioToneFilter::cutoffHz(int8_t value)
{
    const int absoluteValue = value < 0 ? -(int)value : (int)value;
    const float t = (float)absoluteValue / 100.0f;
    const float ratio = kMaxCutoffHz / kMinCutoffHz;
    return value < 0 ? kMaxCutoffHz * powf(1.0f / ratio, t)
                     : kMinCutoffHz * powf(ratio, t);
}

AUDIO_CORE_FLASH void AudioToneFilter::reset()
{
    for (uint8_t channel = 0; channel < 2; channel++)
        clearState(channel);
}

AUDIO_CORE_FLASH void AudioToneFilter::clearState(uint8_t channel)
{
    z1[channel] = 0.0f;
    z2[channel] = 0.0f;
}

bool AudioToneFilter::isSettled(uint8_t channel) const
{
    return fabsf(z1[channel]) < kStateSettleThreshold &&
           fabsf(z2[channel]) < kStateSettleThreshold;
}

AUDIO_CORE_FLASH void AudioToneFilter::setValue(int8_t value)
{
    if (value > 100)
        value = 100;
    else if (value < -100)
        value = -100;

    if (value == 0)
    {
        active = false;
        return;
    }

    float cutoff = cutoffHz(value);
    const float cutoffLimit = 0.49f * AUDIO_SAMPLE_RATE_EXACT;
    if (cutoff > cutoffLimit)
        cutoff = cutoffLimit;

    const float pi = 3.14159265358979323846f;
    const float omega = 2.0f * pi * cutoff / AUDIO_SAMPLE_RATE_EXACT;
    const float cosine = cosf(omega);
    const float alpha = sinf(omega) * 0.70710678118f;

    float nextB0;
    float nextB1;
    float nextB2;
    if (value < 0)
    {
        nextB0 = (1.0f - cosine) * 0.5f;
        nextB1 = 1.0f - cosine;
        nextB2 = nextB0;
    }
    else
    {
        nextB0 = (1.0f + cosine) * 0.5f;
        nextB1 = -(1.0f + cosine);
        nextB2 = nextB0;
    }

    const float a0 = 1.0f + alpha;
    const float nextA1 = -2.0f * cosine;
    const float nextA2 = 1.0f - alpha;
    if (!(a0 > 0.0f) || !isfinite(nextB0) || !isfinite(nextB1) || !isfinite(nextB2) ||
        !isfinite(nextA1) || !isfinite(nextA2))
    {
        active = false;
        return;
    }

    const bool wasActive = active;
    const float inverseA0 = 1.0f / a0;
    b0 = nextB0 * inverseA0;
    b1 = nextB1 * inverseA0;
    b2 = nextB2 * inverseA0;
    a1 = nextA1 * inverseA0;
    a2 = nextA2 * inverseA0;

    if (!wasActive)
        reset();
    active = true;
}

AUDIO_CORE_RAM void AudioToneFilter::update()
{
    audio_block_t* input[2] = { receiveReadOnly(0), receiveReadOnly(1) };

    if (!active)
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

    const float cb0 = b0;
    const float cb1 = b1;
    const float cb2 = b2;
    const float ca1 = a1;
    const float ca2 = a2;

    for (uint8_t channel = 0; channel < 2; channel++)
    {
        if (input[channel] == nullptr && isSettled(channel))
        {
            clearState(channel);
            continue;
        }

        audio_block_t* output = nullptr;
        if (input[channel] != nullptr)
            output = input[channel]->ref_count == 1 ? input[channel] : allocate();
        else
            output = allocate();

        const int16_t* source = input[channel] != nullptr ? input[channel]->data : nullptr;
        int16_t* destination = output != nullptr ? output->data : nullptr;
        float state1 = z1[channel];
        float state2 = z2[channel];

        for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; sample++)
        {
            const float x = source != nullptr ? (float)source[sample] : 0.0f;
            const float y = cb0 * x + state1;
            state1 = cb1 * x - ca1 * y + state2;
            state2 = cb2 * x - ca2 * y;

            if (destination != nullptr)
            {
                float outputSample = y;
                if (outputSample > 32767.0f)
                    outputSample = 32767.0f;
                else if (outputSample < -32768.0f)
                    outputSample = -32768.0f;
                destination[sample] = (int16_t)outputSample;
            }
        }

        z1[channel] = state1;
        z2[channel] = state2;
        if (isSettled(channel))
            clearState(channel);

        if (output != nullptr)
        {
            transmit(output, channel);
            if (output != input[channel])
                release(output);
        }
        if (input[channel] != nullptr)
            release(input[channel]);
    }
}
