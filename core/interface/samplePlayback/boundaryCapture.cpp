#include "samplePlayback/boundaryCapture.h"

#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE

#include "mtAudioEngine.h"
#include "samplePlayback/previewSession.h"
#include "samplePlayback/samplePlayback.h"
#include "sampleBoundaryLogic.h"
#include "fileManager.h"
#include "core/interface.h"

extern strMtProject mtProject;

using namespace sampleBoundary;

namespace
{
struct State
{
    PreviewSession session;
    bool finalizing = false;
    uint8_t mode = playModeSingleShot; // mode at session start; a different mode at commit discards
    Capture captures[kBoundaryCount];
    uint32_t nextOrder = 1;
} s;

bool modeSupported(uint8_t m)
{
    return m == playModeSingleShot || m == playModeLoopForward || m == playModeLoopBackward ||
           m == playModePingpong;
}

Mode toBoundaryMode(uint8_t m)
{
    switch (m)
    {
        case playModeLoopForward: return kForwardLoop;
        case playModeLoopBackward: return kBackwardLoop;
        case playModePingpong: return kPingpongLoop;
        default: return kOneShot;
    }
}

void clearState()
{
    s.session.clear();
    for (uint8_t k = 0; k < kBoundaryCount; k++)
        s.captures[k] = Capture();
    s.nextOrder = 1;
}

// The single commit path: stores all four values and refreshes the audio side while the audio update
// interrupt is held off, so neither the audio engine nor a note-on ever sees a mixture of old and new
// boundaries. Does not touch the incremental editors, zero crossings or any other instrument field.
void commit(uint8_t instrIdx, const Arrangement& a, bool loopMode)
{
    strInstrument& instr = mtProject.instrument[instrIdx];

    AudioNoInterrupts();
    instr.startPoint = a.pos[kStart];
    instr.endPoint = a.pos[kEnd];
    if (loopMode)
    {
        instr.loopPoint1 = a.pos[kLoopStart];
        instr.loopPoint2 = a.pos[kLoopEnd];
        for (uint8_t i = 0; i < 8; i++)
        {
            instrumentPlayer[i].applyCapturedBoundaries(instrIdx, a.pos[kLoopStart], a.pos[kLoopEnd]);
        }
    }
    AudioInterrupts();

    newFileManager.setInstrumentStructChanged(instrIdx);
}

void refreshUi(uint8_t instrIdx, bool loopMode)
{
    cSamplePlayback& sp = samplePlayback;
    if (&mtProject.instrument[instrIdx] != sp.editorInstrument || !sp.moduleRefresh)
        return;

    sp.refreshPoints = 1;
    sp.refreshSpectrum = 1;
    sp.showStartPointValue();
    sp.showEndPointValue();
    if (loopMode)
    {
        sp.showLoopPoint1Value();
        sp.showLoopPoint2Value();
    }
}
} // namespace

namespace boundaryCapture
{
void onNoteOn(int16_t source)
{
    if (s.session.active)
        return; // never replace a running session
    if (mtProject.values.lastUsedInstrument >= kSampleInstrumentsCount)
        return;

    const strInstrument& instr = mtProject.instrument[mtProject.values.lastUsedInstrument];
    if (!modeSupported(instr.playMode) || instr.sample.length < 2)
        return;

    PreviewSession candidate;
    if (!candidate.begin(source))
        return;

    clearState();
    s.session = candidate;
    s.mode = instr.playMode;
}

bool capture(uint8_t kind)
{
    if (!s.session.active || s.finalizing || kind >= kBoundaryCount)
        return false;

    if (!s.session.stillValid())
    {
        finalize(true);
        return false;
    }

    // Loop Start / Loop End are not active boundaries in One-Shot.
    if (s.mode == playModeSingleShot && (kind == kLoopStart || kind == kLoopEnd))
        return false;

    Capture& c = s.captures[kind];
    c.present = true;
    c.value = s.session.position();
    c.order = s.nextOrder++; // a repeated key becomes the newest submission
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
    bool anyPending = false;
    for (uint8_t k = 0; k < kBoundaryCount; k++)
        anyPending |= s.captures[k].present;

    if (anyPending && instrIdx < kSampleInstrumentsCount &&
        mtProject.instrument[instrIdx].playMode == s.mode)
    {
        const strInstrument& instr = mtProject.instrument[instrIdx];

        Context ctx;
        ctx.sampleFrames = instr.sample.length;
        ctx.mode = toBoundaryMode(s.mode);
        ctx.minForwardLoopFrames = playerEngine::getForwardLoopMinFrames();

        Arrangement current;
        current.pos[kStart] = instr.startPoint;
        current.pos[kLoopStart] = instr.loopPoint1;
        current.pos[kLoopEnd] = instr.loopPoint2;
        current.pos[kEnd] = instr.endPoint;

        Arrangement resolved;
        resolve(current, s.captures, ctx, resolved);

        const bool loopMode = ctx.mode != kOneShot;
        bool changed = resolved.pos[kStart] != current.pos[kStart] ||
                       resolved.pos[kEnd] != current.pos[kEnd];
        if (loopMode)
        {
            changed = changed || resolved.pos[kLoopStart] != current.pos[kLoopStart] ||
                      resolved.pos[kLoopEnd] != current.pos[kLoopEnd];
        }

        if (changed)
        {
            commit(instrIdx, resolved, loopMode);
            if (refresh)
                refreshUi(instrIdx, loopMode);
        }
    }

    clearState();
    s.finalizing = false;
}
} // namespace boundaryCapture

#endif // MOD_SAMPLE_BOUNDARY_CAPTURE
