/*
 * LXR DrumVoice PSYN proof of concept.
 *
 * Contains the original Erica Synths LXR-02 DrumVoice DSP dependency closure,
 * adapted to an instance-owned state and the Tracker PSYN host boundary.
 * Upstream baseline: erica-synths/lxr-desktop-app
 * dca0a44413931a12de65941355f06c07598a9be5.
 *
 * The modulation-node system, LXR sequencer, UI, parameter pointer table,
 * mixer, pan routing, and hardware fader are intentionally excluded.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * See LICENSE-LXR-GPL-3.0.txt and NOTICE-LXR.md.
 */

#include "psyn/psyn_cpp_bridge.h"
#include "PolySynth/sdk/InstrumentVoice.h"
#include "PolySynth/sdk/SynthVoiceSDK.h"
#include "Soundengine/DSPAudio/DrumVoice.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace polysynth
{

class LxrDrumPocVoice final : public InstrumentVoice
{
  public:
    LxrDrumPocVoice() { reset(); }

    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
    {
        metadata.params.clear();
        metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
    }

    void onMidiEvent(const InstrumentVoiceContext& /*ctx*/, const MidiEvent& event) final
    {
        if (event.type == MidiEvent::Type::kNoteOn && event.data.note.velocity > 0)
        {
            // Single-part POC trigger mapping. This is a Tracker adaptation,
            // not the original LXR sequencer's internal part routing.
            if (event.data.note.note == kTriggerNote)
            {
                lxr_drum_voice_trigger(&voice, event.data.note.velocity, event.data.note.note);
                heldNote = event.data.note.note;
                held = true;
                lastNote = event.data.note.note;
                lastNoteValid = true;
                currentVelocity = static_cast<float>(event.data.note.velocity) / 127.0f;
                currentVelocityValid = true;
            }
        }
        else if (event.type == MidiEvent::Type::kNoteOff ||
                 (event.type == MidiEvent::Type::kNoteOn && event.data.note.velocity == 0))
        {
            if (held && event.data.note.note == heldNote)
            {
                held = false;
            }
        }
        else if (event.type == MidiEvent::Type::kControlChange &&
                 event.data.controlChange.id == MidiEvent::Data::ControlChange::kAllNotesOffCC)
        {
            held = false;
        }
    }

    void onParamChange(int /*paramId*/, Patch::optional_paramvalue_t /*value*/) final
    {
        // The first extraction milestone intentionally has no parameter UI.
    }

    void renderVoiceAudio(const InstrumentVoiceContext& /*ctx*/,
                          std::optional<AudioBufferStereoRef> /*inputBuffer*/,
                          AudioBufferStereoRef outputBuffer) final
    {
        auto left = outputBuffer[0];
        auto right = outputBuffer[1];
        std::fill(left.begin(), left.end(), 0.0f);
        std::fill(right.begin(), right.end(), 0.0f);

        // LXR's default OUTPUT_DMA_SIZE is 64. Tracker renders 128 samples
        // per callback, so preserve the original control/audio cadence by
        // updating and rendering two 64-sample LXR blocks.
        std::array<int16_t, kLxrBlockSize> block{};
        size_t offset = 0;
        while (offset < left.size())
        {
            const size_t count = std::min(kLxrBlockSize, left.size() - offset);
            lxr_drum_voice_update(&voice);
            lxr_drum_voice_render_block(&voice, block.data(), static_cast<uint8_t>(count));

            for (size_t i = 0; i < count; ++i)
            {
                const float sample = static_cast<float>(block[i]) * (1.0f / 32768.0f);
                left[offset + i] = sample;
                right[offset + i] = sample;
            }
            offset += count;
        }
    }

    bool isActive() final
    {
        const bool envelopeActive =
            voice.ampFilterInput > kActivityThreshold ||
            voice.lastGain > kActivityThreshold ||
            voice.oscVolEg.value > kActivityThreshold;
        const bool transientActive =
            voice.transGen.waveform >= 2 && voice.transGen.phase < kTransientEndPhase;
        return envelopeActive || transientActive;
    }

    bool handlesNote(note_t note) final { return held && note == heldNote; }
    bool handlesAnyNote() final { return held; }

    tiny_optional<note_t> getLastNote() final
    {
        return lastNoteValid ? tiny_optional<note_t>(lastNote) : tiny_optional<note_t>{};
    }

    tiny_optional<float> getCurrentNoteVelocity() final
    {
        return (currentVelocityValid && isActive()) ? tiny_optional<float>(currentVelocity) : tiny_optional<float>{};
    }

    void reset() final
    {
        voice = {};
        lxr_drum_voice_init(&voice, 0);
        heldNote = 0;
        held = false;
        lastNote = 0;
        currentVelocity = 0.0f;
        lastNoteValid = false;
        currentVelocityValid = false;
    }

  private:
    static constexpr uint8_t kTriggerNote = 36;
    static constexpr size_t kLxrBlockSize = 64;
    static constexpr float kActivityThreshold = 0.0001f;
    static constexpr uint32_t kTransientEndPhase = 2311061504u;

    DrumVoice voice{};
    note_t heldNote = 0;
    note_t lastNote = 0;
    float currentVelocity = 0.0f;
    bool held = false;
    bool lastNoteValid = false;
    bool currentVelocityValid = false;
};

} // namespace polysynth

PSYN_REGISTER_CPP_VOICE(polysynth::LxrDrumPocVoice)
