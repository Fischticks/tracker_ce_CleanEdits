#include "samplePlayback/lazyChop.h"

#ifdef MOD_LAZY_CHOP

#include "VoiceStreamManager.h"
#include "core/interface.h"
#include "fileManager.h"
#include "mtAudioEngine.h"
#include "mtSliceManager.h"
#include "samplePlayback/previewSession.h"
#include "samplePlayback/samplePlayback.h"

extern strMtProject mtProject;

namespace
{
struct State
{
    PreviewSession session;
    bool finalizing = false;
    uint16_t pending[MAX_SLICE_NUMBER];
    uint8_t pendingCount = 0;
} s;

void clearState()
{
    s.session.clear();
    s.pendingCount = 0;
}
} // namespace

namespace lazyChop
{
void onNoteOn(int16_t source)
{
    if (s.session.active) // never silently replace a running session
        return;
    if (mtProject.values.lastUsedInstrument >= kSampleInstrumentsCount)
        return;

    cSamplePlayback& sp = samplePlayback;
    const strInstrument& instr = mtProject.instrument[mtProject.values.lastUsedInstrument];
    if (instr.playMode != playModeSlice && instr.playMode != playModeBeatSlice)
        return;
    if (sp.sliceScreenMode2) // alternate layout: Add Slice is not on screen
        return;

    PreviewSession candidate;
    if (!candidate.begin(source))
        return;
    if (candidate.isReversed()) // captured positions and insertion assume forward playback
        return;

    clearState();
    s.session = candidate;
}

bool capture()
{
    if (!s.session.active || s.finalizing)
        return false;

    if (!s.session.stillValid())
    {
        finalize(true);
        return false; // session over: the ordinary midpoint Add applies
    }

    if (s.session.isReversed())
        return true; // consumed, nothing captured

    const strInstrument& instr = mtProject.instrument[s.session.instrumentIndex];
    const uint16_t pos = s.session.position();

    // Capacity: existing boundaries (an empty table gets a zero boundary on first insert) + pending.
    const uint8_t existing = instr.sliceNumber == 0 ? 1 : instr.sliceNumber;
    if (existing + s.pendingCount >= MAX_SLICE_NUMBER)
        return true;

    for (uint8_t i = 0; i < s.pendingCount; i++)
    {
        if (s.pending[i] == pos) // exact duplicate
            return true;
    }

    s.pending[s.pendingCount++] = pos;
    return true;
}

void onNoteOff(int16_t source)
{
    if (s.session.active && s.session.noteSource == source)
        finalize(true);
}

void update()
{
    if (!s.session.active || s.finalizing)
        return;
    if (!s.session.stillValid())
        finalize(true);
}

void finalize(bool refresh)
{
    if (!s.session.active || s.finalizing)
        return;
    s.finalizing = true;

    const uint8_t instrIdx = s.session.instrumentIndex;
    if (s.pendingCount > 0 && instrIdx < kSampleInstrumentsCount)
    {
        strInstrument* instr = &mtProject.instrument[instrIdx];
        const uint8_t inserted = sliceManager.insertSlicesAtPositions(instr, s.pending, s.pendingCount);

        if (inserted > 0)
        {
            if (mtProject.sampleMemory[instrIdx].isStreamed)
            {
                // Never rebuild while a voice may be reading the shared buffers.
                voiceStreamManager.requestInstrumentSliceCacheRefresh(instrIdx);
            }

            newFileManager.setInstrumentStructChanged(instrIdx);

            cSamplePlayback& sp = samplePlayback;
            if (refresh && instr == sp.editorInstrument && sp.moduleRefresh)
            {
                sp.zoom.zoomPosition = instr->slices[instr->selectedSlice];
                if ((sp.zoom.zoomPosition > sp.zoom.zoomEnd) || (sp.zoom.zoomPosition < sp.zoom.zoomStart))
                {
                    sp.refreshSpectrum = 1;
                }
                sp.showSlicesSelectValue();
                sp.showSlicesAdjustValue();
                sp.refreshSlicePoints = 1;
            }
        }
    }

    clearState(); // cleared only after the commit has completed
    s.finalizing = false;
}
} // namespace lazyChop

#endif // MOD_LAZY_CHOP
