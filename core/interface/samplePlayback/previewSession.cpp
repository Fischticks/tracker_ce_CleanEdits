#include "samplePlayback/previewSession.h"

#if defined(MOD_LAZY_CHOP) || defined(MOD_SAMPLE_BOUNDARY_CAPTURE)

#include "mtAudioEngine.h"
#include "mtSequencer.h"
#include "mtStructs.h"

extern strMtProject mtProject;

bool PreviewSession::begin(int16_t source)
{
    if (mtProject.values.lastUsedInstrument >= kSampleInstrumentsCount)
        return false;

    const uint8_t voice = engine.getLastUsedVoice();
    if (voice >= 8)
        return false;

    int16_t seqSource = -1;
    uint8_t seqInstrument = 0;
    if (!sequencer.getManualNoteBinding(voice, seqSource, seqInstrument))
        return false;
    if (seqSource != source || seqInstrument != mtProject.values.lastUsedInstrument)
        return false;
    if (!instrumentPlayer[voice].isSamplePlaying())
        return false;

    active = true;
    voiceIndex = voice;
    instrumentIndex = mtProject.values.lastUsedInstrument;
    noteSource = source;
    return true;
}

bool PreviewSession::stillValid() const
{
    if (!active || voiceIndex >= 8)
        return false;
    if (mtProject.values.lastUsedInstrument != instrumentIndex)
        return false;

    int16_t seqSource = -1;
    uint8_t seqInstrument = 0;
    if (!sequencer.getManualNoteBinding(voiceIndex, seqSource, seqInstrument))
        return false;
    if (seqSource != noteSource || seqInstrument != instrumentIndex)
        return false;

    return instrumentPlayer[voiceIndex].isSamplePlaying();
}

bool PreviewSession::isReversed() const
{
    return instrumentPlayer[voiceIndex].isSampleReversed();
}

uint16_t PreviewSession::position() const
{
    return instrumentPlayer[voiceIndex].getWavePosition();
}

#endif
