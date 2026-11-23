/*
 * Host-side smoke test for the extracted LXR DrumVoice backend.
 * SPDX-License-Identifier: GPL-3.0-only
 * This test validates the Tracker port boundary; it is not an audio
 * equivalence test against the original LXR firmware.
 */
#include "Soundengine/DSPAudio/DrumVoice.h"

#include <stdint.h>
#include <stdio.h>

enum { PART_COUNT = 3 };

typedef struct
{
    int16_t samples[3];
    int16_t guard;
} GuardedOddBlock;

static int32_t abs_sample(int16_t value)
{
    const int32_t wide = value;
    return wide < 0 ? -wide : wide;
}

static int16_t saturating_add(int16_t a, int16_t b)
{
    const int32_t sum = (int32_t)a + (int32_t)b;
    return (int16_t)(sum > 32767 ? 32767 : (sum < -32768 ? -32768 : sum));
}

static int fail(const char* message)
{
    fprintf(stderr, "LXR DrumVoice smoke test failed: %s\n", message);
    return 1;
}

int main(void)
{
    DrumVoice parts[PART_COUNT] = { 0 };
    const uint8_t triggerNotes[PART_COUNT] = { 36, 37, 38 };
    const uint8_t velocities[PART_COUNT] = { 127, 96, 80 };
    const uint8_t expectedWaveforms[PART_COUNT] = { TRI, SAW, REC };

    for (uint8_t part = 0; part < PART_COUNT; ++part)
    {
        lxr_drum_voice_init(&parts[part], part);
        if (parts[part].mixOscs != 1 ||
            parts[part].osc.waveform != expectedWaveforms[part] ||
            parts[part].vol <= 0.0f)
        {
            return fail("part initialization did not establish the expected source-backed defaults");
        }
        lxr_drum_voice_trigger(&parts[part], velocities[part], triggerNotes[part]);
    }

    int16_t partBuffer[OUTPUT_DMA_SIZE] = { 0 };
    int16_t mixedBuffer[OUTPUT_DMA_SIZE] = { 0 };
    uint64_t energy = 0;
    int32_t peak = 0;

    // Exercise all three independently owned voices simultaneously. Preserve
    // the original mix order and saturating accumulation used by the host mix.
    for (unsigned blockIndex = 0; blockIndex < 32; ++blockIndex)
    {
        for (uint8_t part = 0; part < PART_COUNT; ++part)
        {
            lxr_drum_voice_update(&parts[part]);
        }

        for (unsigned i = 0; i < OUTPUT_DMA_SIZE; ++i)
        {
            mixedBuffer[i] = 0;
        }

        for (uint8_t part = 0; part < PART_COUNT; ++part)
        {
            lxr_drum_voice_render_block(&parts[part], partBuffer, OUTPUT_DMA_SIZE);
            for (unsigned i = 0; i < OUTPUT_DMA_SIZE; ++i)
            {
                mixedBuffer[i] = saturating_add(mixedBuffer[i], partBuffer[i]);
            }
        }

        for (unsigned i = 0; i < OUTPUT_DMA_SIZE; ++i)
        {
            const int32_t magnitude = abs_sample(mixedBuffer[i]);
            energy += (uint32_t)magnitude;
            if (magnitude > peak)
            {
                peak = magnitude;
            }
        }
    }

    if (energy == 0 || peak == 0)
    {
        return fail("the three-part DrumVoice render/mix path produced no nonzero samples");
    }

    // Verify that phase changes to one part do not mutate another part.
    const uint32_t part0StartPhase = parts[0].osc.startPhase;
    lxr_drum_voice_set_phase(&parts[1], 64);
    if (parts[0].osc.startPhase != part0StartPhase ||
        parts[1].osc.startPhase == part0StartPhase)
    {
        return fail("separate DrumVoice states are not independently owned");
    }

    // Exercise odd and single-sample host blocks with a canary after the buffer.
    GuardedOddBlock odd = { { 123, 456, 789 }, (int16_t)0x5a5a };
    lxr_drum_voice_init(&parts[2], 2);
    lxr_drum_voice_trigger(&parts[2], 100, 38);
    lxr_drum_voice_update(&parts[2]);
    lxr_drum_voice_render_block(&parts[2], odd.samples, 3);
    if (odd.guard != (int16_t)0x5a5a)
    {
        return fail("three-sample render wrote beyond the caller buffer");
    }

    struct
    {
        int16_t sample;
        int16_t guard;
    } single = { 321, (int16_t)0x5a5a };
    lxr_drum_voice_update(&parts[2]);
    lxr_drum_voice_render_block(&parts[2], &single.sample, 1);
    if (single.guard != (int16_t)0x5a5a)
    {
        return fail("single-sample render wrote beyond the caller buffer");
    }

    printf("LXR three-part DrumVoice smoke test passed: energy=%llu peak=%ld, independent state and odd block guards verified\n",
           (unsigned long long)energy,
           (long)peak);
    return 0;
}
