/* Portable seeded RNG adapter; original firmware uses STM32 hardware RNG.
 * This changes noise sequence and is documented as a platform adaptation.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdint.h>
#include "Soundengine/DSPAudio/random.h"
static uint32_t lxr_rng_state=0x6d2b79f5u;
void initRng(void){lxr_rng_state=0x6d2b79f5u;}
uint32_t GetRngValue(void){uint32_t x=lxr_rng_state;x^=x<<13;x^=x>>17;x^=x<<5;lxr_rng_state=x;return x;}
