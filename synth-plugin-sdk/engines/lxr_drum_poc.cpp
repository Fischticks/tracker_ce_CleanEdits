/*
 * LXR DrumVoice PSYN proof of concept.
 *
 * Contains the original Erica Synths LXR-02 DrumVoice DSP dependency closure,
 * adapted to instance-owned state and the Tracker PSYN host boundary.
 * Upstream baseline: erica-synths/lxr-desktop-app
 * dca0a44413931a12de65941355f06c07598a9be5.
 *
 * This POC owns three independent original DrumVoice states inside one
 * Tracker InstrumentVoice. LXR modulation nodes, LFO routing, sequencer,
 * parameter pointer table, UI, mixer/pan routing, and hardware fader remain
 * intentionally excluded.
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
            const int part = partForNote(event.data.note.note);
            if (part >= 0)
            {
                const size_t index = static_cast<size_t>(part);
                lxr_drum_voice_trigger(&voices[index], event.data.note.velocity, event.data.note.note);
                heldNotes[index] = event.data.note.note;
                held[index] = true;
                pendingTrigger[index] = true;
                partVelocity[index] = static_cast<float>(event.data.note.velocity) / 127.0f;
                triggerOrder[index] = ++triggerCounter;
                lastNote = event.data.note.note;
                lastNoteValid = true;
            }
        }
        else if (event.type == MidiEvent::Type::kNoteOff ||
                 (event.type == MidiEvent::Type::kNoteOn && event.data.note.velocity == 0))
        {
            const int part = partForNote(event.data.note.note);
            if (part >= 0)
            {
                const size_t index = static_cast<size_t>(part);
                if (held[index] && heldNotes[index] == event.data.note.note)
                {
                    // DrumVoice is a one-shot. Note-off releases host note
                    // ownership but does not truncate its amplitude envelope.
                    held[index] = false;
                }
            }
        }
        else if (event.type == MidiEvent::Type::kControlChange &&
                 event.data.controlChange.id == MidiEvent::Data::ControlChange::kAllNotesOffCC)
        {
            held.fill(false);
        }
    }

    void onParamChange(int /*paramId*/, Patch::optional_paramvalue_t /*value*/) final
    {
        // The first three-part extraction milestone intentionally has no editor.
    }

    void renderVoiceAudio(const InstrumentVoiceContext& /*ctx*/,
                          std::optional<AudioBufferStereoRef> /*inputBuffer*/,
                          AudioBufferStereoRef outputBuffer) final
    {
        auto left = outputBuffer[0];
        auto right = outputBuffer[1];
        std::fill(left.begin(), left.end(), 0.0f);
        std::fill(right.begin(), right.end(), 0.0f);

        // The original LXR mixer updates all three DrumVoice envelopes before
        // rendering any part. Preserve that order and its 64-sample cadence.
        std::array<int16_t, kLxrBlockSize> mixed{};
        std::array<int16_t, kLxrBlockSize> partBuffer{};
        size_t offset = 0;
        while (offset < left.size())
        {
            const size_t count = std::min(kLxrBlockSize, left.size() - offset);

            for (size_t part = 0; part < kPartCount; ++part)
            {
                lxr_drum_voice_update(&voices[part]);
            }

            mixed.fill(0);
            for (size_t part = 0; part < kPartCount; ++part)
            {
                lxr_drum_voice_render_block(&voices[part], partBuffer.data(),
                                            static_cast<uint8_t>(count));
                pendingTrigger[part] = false;

                // Match the original mixer's sequential saturating accumulation
                // order, with pan/routing intentionally collapsed to mono.
                for (size_t i = 0; i < count; ++i)
                {
                    mixed[i] = saturatingAdd(mixed[i], partBuffer[i]);
                }
            }

            for (size_t i = 0; i < count; ++i)
            {
                const float sample = static_cast<float>(mixed[i]) * (1.0f / 32768.0f);
                left[offset + i] = sample;
                right[offset + i] = sample;
            }
            offset += count;
        }
    }

    bool isActive() final
    {
        for (size_t part = 0; part < kPartCount; ++part)
        {
            if (isPartActive(part))
            {
                return true;
            }
        }
        return false;
    }

    bool handlesNote(note_t note) final
    {
        const int part = partForNote(note);
        if (part < 0)
        {
            return false;
        }
        const size_t index = static_cast<size_t>(part);
        return held[index] && heldNotes[index] == note;
    }

    bool handlesAnyNote() final
    {
        return std::any_of(held.begin(), held.end(), [](bool value) { return value; });
    }

    tiny_optional<note_t> getLastNote() final
    {
        return lastNoteValid ? tiny_optional<note_t>(lastNote) : tiny_optional<note_t>{};
    }

    tiny_optional<float> getCurrentNoteVelocity() final
    {
        size_t newestActivePart = kPartCount;
        uint32_t newestOrder = 0;
        for (size_t part = 0; part < kPartCount; ++part)
        {
            if (isPartActive(part) && triggerOrder[part] >= newestOrder)
            {
                newestOrder = triggerOrder[part];
                newestActivePart = part;
            }
        }
        return newestActivePart < kPartCount
                   ? tiny_optional<float>(partVelocity[newestActivePart])
                   : tiny_optional<float>{};
    }

    void reset() final
    {
        for (size_t part = 0; part < kPartCount; ++part)
        {
            voices[part] = {};
            lxr_drum_voice_init(&voices[part], static_cast<uint8_t>(part));
        }
        heldNotes.fill(0);
        held.fill(false);
        pendingTrigger.fill(false);
        partVelocity.fill(0.0f);
        triggerOrder.fill(0);
        triggerCounter = 0;
        lastNote = 0;
        lastNoteValid = false;
    }

  private:
    static constexpr size_t kPartCount = 3;
    static constexpr size_t kLxrBlockSize = 64;
    static constexpr float kActivityThreshold = 0.0001f;
    static constexpr uint32_t kTransientEndPhase = 2311061504u;

    // Temporary Tracker-side map: three adjacent notes, replaceable without
    // changing the DSP backend. These values are not source-derived LXR data.
    static constexpr std::array<note_t, kPartCount> kPartNotes = { 36, 37, 38 };

    static int partForNote(note_t note)
    {
        for (size_t part = 0; part < kPartCount; ++part)
        {
            if (kPartNotes[part] == note)
            {
                return static_cast<int>(part);
            }
        }
        return -1;
    }

    static int16_t saturatingAdd(int16_t a, int16_t b)
    {
        const int32_t sum = static_cast<int32_t>(a) + static_cast<int32_t>(b);
        return static_cast<int16_t>(sum > 32767 ? 32767 : (sum < -32768 ? -32768 : sum));
    }

    bool isPartActive(size_t part) const
    {
        const DrumVoice& voice = voices[part];
        const bool envelopeActive =
            voice.ampFilterInput > kActivityThreshold ||
            voice.lastGain > kActivityThreshold ||
            voice.oscVolEg.value > kActivityThreshold;
        const bool transientActive =
            voice.transGen.waveform >= 2 && voice.transGen.phase < kTransientEndPhase;
        return held[part] || pendingTrigger[part] || envelopeActive || transientActive;
    }

    std::array<DrumVoice, kPartCount> voices{};
    std::array<note_t, kPartCount> heldNotes{};
    std::array<bool, kPartCount> held{};
    std::array<bool, kPartCount> pendingTrigger{};
    std::array<float, kPartCount> partVelocity{};
    std::array<uint32_t, kPartCount> triggerOrder{};
    uint32_t triggerCounter = 0;
    note_t lastNote = 0;
    bool lastNoteValid = false;
};

} // namespace polysynth

PSYN_REGISTER_CPP_VOICE(polysynth::LxrDrumPocVoice)
