/*
 * Host-side smoke test for the extracted LXR DrumVoice backend.
 * SPDX-License-Identifier: GPL-3.0-only
 * This test validates the Tracker port boundary; it is not an audio
 * equivalence test against the original LXR firmware.
 */
#include "Soundengine/DSPAudio/DrumVoice.h"

#include <stdint.h>
#include <stdio.h>

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

static int fail(const char* message)
{
    fprintf(stderr, "LXR DrumVoice smoke test failed: %s\n", message);
    return 1;
}

int main(void)
{
    DrumVoice voice = { 0 };
    lxr_drum_voice_init(&voice, 0);

    if (voice.mixOscs != 1 || voice.osc.waveform != TRI || voice.vol <= 0.0f)
    {
        return fail("initialization did not establish the expected source-backed voice defaults");
    }

    lxr_drum_voice_trigger(&voice, 127, 36);
    if (voice.velo < 0.999f)
    {
        return fail("trigger did not capture full velocity");
    }

    int16_t block[OUTPUT_DMA_SIZE] = { 0 };
    uint64_t energy = 0;
    int32_t peak = 0;
    for (unsigned blockIndex = 0; blockIndex < 32; ++blockIndex)
    {
        lxr_drum_voice_update(&voice);
        lxr_drum_voice_render_block(&voice, block, OUTPUT_DMA_SIZE);
        for (unsigned i = 0; i < OUTPUT_DMA_SIZE; ++i)
        {
            const int32_t magnitude = abs_sample(block[i]);
            energy += (uint32_t)magnitude;
            if (magnitude > peak)
            {
                peak = magnitude;
            }
        }
    }

    if (energy == 0 || peak == 0)
    {
        return fail("the original DrumVoice render path produced no nonzero samples");
    }

    // Verify that the explicit state API does not alias separate voice state.
    DrumVoice secondVoice = { 0 };
    lxr_drum_voice_init(&secondVoice, 1);
    lxr_drum_voice_set_phase(&secondVoice, 64);
    if (voice.osc.startPhase == secondVoice.osc.startPhase ||
        secondVoice.osc.waveform != SAW)
    {
        return fail("separate DrumVoice states are not independently owned");
    }

    // Exercise odd and single-sample host blocks with a canary after the buffer.
    GuardedOddBlock odd = { { 123, 456, 789 }, (int16_t)0x5a5a };
    lxr_drum_voice_init(&secondVoice, 0);
    lxr_drum_voice_trigger(&secondVoice, 100, 36);
    lxr_drum_voice_update(&secondVoice);
    lxr_drum_voice_render_block(&secondVoice, odd.samples, 3);
    if (odd.guard != (int16_t)0x5a5a)
    {
        return fail("three-sample render wrote beyond the caller buffer");
    }

    struct
    {
        int16_t sample;
        int16_t guard;
    } single = { 321, (int16_t)0x5a5a };
    lxr_drum_voice_update(&secondVoice);
    lxr_drum_voice_render_block(&secondVoice, &single.sample, 1);
    if (single.guard != (int16_t)0x5a5a)
    {
        return fail("single-sample render wrote beyond the caller buffer");
    }

    printf("LXR DrumVoice smoke test passed: energy=%llu peak=%ld, independent state and odd block guards verified\n",
           (unsigned long long)energy,
           (long)peak);
    return 0;
}
