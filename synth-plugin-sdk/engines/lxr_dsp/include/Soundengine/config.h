#pragma once
#include <stdint.h>
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
/* Tracker host sample rate replaces the LXR hardware clock rate (44,140 Hz). */
#define REAL_FS 44100.0f
#define REAL_FS2 (REAL_FS * 0.5f)
/* Preserve the pinned LXR large-buffer control/envelope scaling. */
#define USE_BIG_SAMPLE_BUFFER 1
#define OUTPUT_DMA_SIZE 64
#define AMP_EG_SYNC 0
#define ENABLE_MIX_OSC 1
#define ENABLE_DRUM_SVF 1
#define SMOOTH_MIXER_VOL_PAN_DRIVE 1
#define USE_FILTER_DRIVE 0
#define INTERPOLATE_OSC 1
#define INTERPOLATE_FM_OSC 1
#define EG_SPEED 1
#define PITCH_AMOUNT_FACTOR 32
#define IN_ITCM_RAM
#define INDTCM
#define INDTCMZ
