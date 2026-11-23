/*
 * LXR-02-inspired Tracker drum proof of concept.
 *
 * This Tracker-facing prototype is informed by the audited LXR-02 DrumVoice
 * architecture at Erica Synths lxr-desktop-app revision
 * dca0a44413931a12de65941355f06c07598a9be5. It is an initial independent
 * approximation, not a source-faithful port and not an official Erica Synths
 * product or endorsed implementation.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * See ../LICENSE-LXR-GPL-3.0.txt and ../NOTICE-LXR.md for applicable terms.
 */

#include "psyn/psyn_cpp_bridge.h"
#include "PolySynth/sdk/InstrumentVoice.h"
#include "PolySynth/sdk/SynthVoiceSDK.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace polysynth
{
namespace
{
enum Param : int
{
    kMode = 0,
    kTune,
    kFmAmount,
    kPitchSweep,
    kFilterCutoff,
    kDecay,
    kParamsPerPart,
};

constexpr int kPartCount = 3;
constexpr int kParamCount = kPartCount * kParamsPerPart;
constexpr std::array<uint8_t, kPartCount> kTriggerNotes = {36, 38, 42};

struct DrumPart
{
    float carrierPhase = 0.0f;
    float modPhase = 0.0f;
    float amp = 0.0f;
    float pitchEnv = 0.0f;
    float filterState = 0.0f;
    float tune = 0.0f;
    float fm = 0.35f;
    bool fmMode = true;
    float pitchSweep = 0.45f;
    float cutoff = 12000.0f;
    float decaySeconds = 0.24f;
    float velocity = 1.0f;
    uint8_t note = 0;
    bool held = false;
    bool active = false;
};

inline float clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }

inline float midiFrequency(float note)
{
    return 440.0f * std::exp2((note - 69.0f) * (1.0f / 12.0f));
}

inline float wrap01(float phase)
{
    return phase - std::floor(phase);
}
} // namespace

class LxrDrumPocVoice final : public InstrumentVoice
{
  public:
    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
    {
        static constexpr const char* kPartNames[kPartCount] = {"Drum 1", "Drum 2", "Drum 3"};
        static constexpr const char* kParamNames[kParamsPerPart] = {
            "Mode", "Tune", "FM Amount", "Pitch Sweep", "Cutoff", "Decay"
        };
        static constexpr float kDefaults[kParamsPerPart] = {
            1.0f, 0.0f, 0.35f, 0.45f, 12000.0f, 0.24f
        };
        static constexpr float kMins[kParamsPerPart] = {
            0.0f, -24.0f, 0.0f, 0.0f, 80.0f, 0.025f
        };
        static constexpr float kMaxs[kParamsPerPart] = {
            1.0f, 24.0f, 1.0f, 1.0f, 18000.0f, 2.0f
        };

        metadata.params.resize(kParamCount);
        metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
        metadata.maxVoices = 1;
        metadata.supportsIdleSleep = false;

        for (int part = 0; part < kPartCount; ++part)
        {
            Patch::Metadata::EditorLayout::EditorSection section{};
            section.name = kPartNames[part];
            section.shortName = part == 0 ? "D1" : (part == 1 ? "D2" : "D3");
            for (int p = 0; p < kParamsPerPart; ++p)
            {
                const int id = part * kParamsPerPart + p;
                auto& param = metadata.params[id];
                param.displayName = kParamNames[p];
                param.defaultValue = kDefaults[p];
                param.minValue = kMins[p];
                param.maxValue = kMaxs[p];
                param.isEnum = (p == kMode);
                param.editCurve = (p == kFilterCutoff || p == kDecay)
                                    ? Patch::Metadata::Param::Curve::kExponential
                                    : Patch::Metadata::Param::Curve::kLinear;
                section.elements.push_back({
                    .type = Patch::Metadata::EditorLayout::EditorElement::Type::kParam,
                    .id = id,
                });
            }
            metadata.layoutFlat.sections.push_back(section);
        }
    }

    void onMidiEvent(const InstrumentVoiceContext& /*ctx*/, const MidiEvent& event) final
    {
        if (event.type == MidiEvent::Type::kNoteOn && event.data.note.velocity > 0)
        {
            const int part = partForNote(event.data.note.note);
            if (part >= 0)
            {
                trigger(parts[part], event.data.note.note, event.data.note.velocity);
                lastNote = event.data.note.note;
                lastNoteValid = true;
                currentVelocity = static_cast<float>(event.data.note.velocity) / 127.0f;
                currentVelocityValid = true;
            }
        }
        else if (event.type == MidiEvent::Type::kNoteOff ||
                 (event.type == MidiEvent::Type::kNoteOn && event.data.note.velocity == 0))
        {
            for (auto& part : parts)
            {
                if (part.note == event.data.note.note)
                {
                    part.held = false;
                }
            }
        }
        else if (event.type == MidiEvent::Type::kControlChange &&
                 event.data.controlChange.id == MidiEvent::Data::ControlChange::kAllNotesOffCC)
        {
            for (auto& part : parts)
            {
                part.held = false;
            }
        }
    }

    void onParamChange(int paramId, Patch::optional_paramvalue_t value) final
    {
        if (!value || paramId < 0 || paramId >= kParamCount)
        {
            return;
        }
        const int partIndex = paramId / kParamsPerPart;
        const int localParam = paramId % kParamsPerPart;
        auto& part = parts[partIndex];
        const float v = *value;
        switch (localParam)
        {
            case kMode: part.fmMode = (v >= 0.5f); break;
            case kTune: part.tune = v; break;
            case kFmAmount: part.fm = clamp01(v); break;
            case kPitchSweep: part.pitchSweep = clamp01(v); break;
            case kFilterCutoff: part.cutoff = std::clamp(v, 80.0f, 18000.0f); break;
            case kDecay: part.decaySeconds = std::clamp(v, 0.025f, 2.0f); break;
            default: break;
        }
    }

    void renderVoiceAudio(const InstrumentVoiceContext& /*ctx*/,
                          std::optional<AudioBufferStereoRef> /*inputBuffer*/,
                          AudioBufferStereoRef outputBuffer) final
    {
        auto left = outputBuffer[0];
        std::fill(left.begin(), left.end(), 0.0f);

        const float sampleRate = sdk::kSampleRate;
        const float invSampleRate = 1.0f / sampleRate;
        for (size_t i = 0; i < left.size(); ++i)
        {
            float mixed = 0.0f;
            for (auto& part : parts)
            {
                if (!part.active)
                {
                    continue;
                }

                const float pitch = 1.0f + part.pitchSweep * 5.0f * part.pitchEnv;
                const float baseHz = std::clamp(
                    midiFrequency(static_cast<float>(part.note) + part.tune) * pitch,
                    15.0f, sampleRate * 0.40f);
                const float modHz = std::min(baseHz * 2.0f, sampleRate * 0.40f);
                const float mod = std::sin(part.modPhase * 6.28318530718f);
                const float carrierPhase = part.carrierPhase +
                    (part.fmMode ? part.fm * mod * 0.18f : 0.0f);
                const float carrier = std::sin(carrierPhase * 6.28318530718f);
                const float modulator = std::sin(part.modPhase * 6.28318530718f);
                const float mixMode = carrier * (1.0f - part.fm) + modulator * part.fm;
                const float signal = part.fmMode ? carrier : mixMode;

                const float cutoff = std::clamp(part.cutoff, 80.0f, sampleRate * 0.42f);
                const float alpha = 1.0f - std::exp(-6.28318530718f * cutoff * invSampleRate);
                part.filterState += alpha * (signal - part.filterState);

                part.amp *= std::exp(-invSampleRate / part.decaySeconds);
                part.pitchEnv *= 0.9992f;
                const float sample = part.filterState * part.amp * part.velocity * 0.28f;
                mixed += sample;

                part.carrierPhase = wrap01(part.carrierPhase + baseHz * invSampleRate);
                part.modPhase = wrap01(part.modPhase + modHz * invSampleRate);
                if (part.amp < 0.00002f)
                {
                    part.active = false;
                    part.amp = 0.0f;
                    part.filterState = 0.0f;
                }
            }
            const float out = std::clamp(mixed, -0.95f, 0.95f);
            left[i] = out;
        }
    }

    bool isActive() final
    {
        for (const auto& part : parts)
        {
            if (part.active)
            {
                return true;
            }
        }
        return false;
    }

    bool handlesNote(note_t note) final
    {
        for (const auto& part : parts)
        {
            if (part.held && part.note == note)
            {
                return true;
            }
        }
        return false;
    }

    bool handlesAnyNote() final
    {
        for (const auto& part : parts)
        {
            if (part.held)
            {
                return true;
            }
        }
        return false;
    }

    tiny_optional<note_t> getLastNote() final
    {
        return lastNoteValid ? tiny_optional<note_t>(lastNote) : tiny_optional<note_t>{};
    }

    tiny_optional<float> getCurrentNoteVelocity() final
    {
        return currentVelocityValid ? tiny_optional<float>(currentVelocity)
                                    : tiny_optional<float>{};
    }

    void reset() final
    {
        parts = {};
        lastNoteValid = false;
        currentVelocityValid = false;
    }

  private:
    static int partForNote(uint8_t note)
    {
        for (int i = 0; i < kPartCount; ++i)
        {
            if (kTriggerNotes[i] == note)
            {
                return i;
            }
        }
        return -1;
    }

    static void trigger(DrumPart& part, uint8_t note, uint8_t velocity)
    {
        part.note = note;
        part.velocity = static_cast<float>(velocity) / 127.0f;
        part.amp = 1.0f;
        part.pitchEnv = 1.0f;
        part.carrierPhase = 0.0f;
        part.modPhase = 0.0f;
        part.filterState = 0.0f;
        part.held = true;
        part.active = true;
    }

    std::array<DrumPart, kPartCount> parts{};
    note_t lastNote = 0;
    float currentVelocity = 0.0f;
    bool lastNoteValid = false;
    bool currentVelocityValid = false;
};

} // namespace polysynth

PSYN_REGISTER_CPP_VOICE(polysynth::LxrDrumPocVoice)
