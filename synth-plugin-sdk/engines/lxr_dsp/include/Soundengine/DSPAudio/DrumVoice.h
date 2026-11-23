/*
 * DrumVoice state/API adapted from Erica Synths LXR firmware.
 * Source revision: dca0a44413931a12de65941355f06c07598a9be5
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once
#include <stdint.h>
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/Oscillator.h"
#include "Soundengine/DSPAudio/ResonantFilter.h"
#include "Soundengine/DSPAudio/distortion.h"
#include "Soundengine/DSPAudio/SlopeEg2.h"
#include "Soundengine/DSPAudio/Decay.h"
#include "Soundengine/DSPAudio/1PoleLp.h"
#include "Soundengine/DSPAudio/transientGenerator.h"
#include "Soundengine/DSPAudio/snapEg.h"
typedef struct VoiceStruct {
  OscInfo osc;
  OscInfo modOsc;
  float fmModAmount;
  float vol;
  float velo;
  int16_t oscSample;
  DecayEg oscPitchEg;
  float egPitchModAmount;
  float offset;
  TransientGenerator transGen;
  SlopeEg2 oscVolEg;
  float egValueOscVol;
  Distortion distortion;
  ResonantFilter filter;
  uint8_t filterType;
  double ampFilterInput;
  double lastGain;
  uint8_t mixOscs;
  float decimationCnt;
  float decimationRate;
  SnapEg snapEg;
  uint8_t volumeMod;
  float combinedVolume;
  OnePoleFilter FaderVolumeFilter;
  OnePoleFilter MixerPageFilter[2];
  float pwm;
} DrumVoice;
void lxr_drum_voice_init(DrumVoice* voice, uint8_t originalVoiceIndex);
void lxr_drum_voice_set_phase(DrumVoice* voice, uint8_t phase);
void lxr_drum_voice_trigger(DrumVoice* voice, uint8_t velocity, uint8_t note);
void lxr_drum_voice_update(DrumVoice* voice);
void lxr_drum_voice_render_block(DrumVoice* voice, int16_t* buffer, uint8_t size);
