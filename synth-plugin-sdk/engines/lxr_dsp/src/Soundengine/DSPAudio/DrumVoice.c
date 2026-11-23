/*
 * DrumVoice.c
 *
 *  Created on: 03.04.2012
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */


#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Oscillator.h"
#include "math.h"
#include "Soundengine/DSPAudio/BufferTools.h"
//---------------------------------------------------
//---------------------------------------------------

//---------------------------------------------------
void lxr_drum_voice_set_phase(DrumVoice* voice, const uint8_t phase)
{
	const uint32_t startPhase = (phase/127.f)*0xffffffff;
	(*voice).osc.startPhase = startPhase;
	(*voice).modOsc.startPhase = startPhase;
}
//---------------------------------------------------
void lxr_drum_voice_init(DrumVoice* voice, uint8_t voiceIndex)
{
	initOnePole(&(*voice).FaderVolumeFilter);
		setOnePoleCoef(&(*voice).FaderVolumeFilter,0.01);

#if SMOOTH_MIXER_VOL_PAN_DRIVE
		for(uint8_t j=0;j<2;j++)
		{
			initOnePole(&(*voice).MixerPageFilter[j]);
			setOnePoleCoef(&(*voice).MixerPageFilter[j],0.01);
		}
		initOnePole(&(*voice).distortion.DriveParamFilter);
		setOnePoleCoef(&(*voice).distortion.DriveParamFilter,0.01);

#endif

		SnapEg_init(&(*voice).snapEg);
		(*voice).vol = 0.8f;
		//(*voice).panModifier = 1.f;
		(*voice).fmModAmount = 0.5f;
		transient_init(&(*voice).transGen);
#if ENABLE_DRUM_SVF
		SVF_init(&(*voice).filter);
		(*voice).filterType = 0x01;
#endif
		(*voice).modOsc.freq = 440;
		(*voice).modOsc.waveform = 1;
		(*voice).modOsc.fmMod = 0;
		(*voice).modOsc.midiFreq = 70<<8 | 64;
		(*voice).modOsc.pitchMod = 1.0f;
		(*voice).modOsc.modNodeValue = 1;

		(*voice).volumeMod = 1;

		(*voice).osc.freq = 440;
		(*voice).osc.modNodeValue = 1;
		(*voice).osc.waveform = TRI+voiceIndex; //for testing init to tri,saw,rec
		(*voice).osc.fmMod = 0;
		(*voice).osc.midiFreq = 70<<8 | 64;

		DecayEg_init(&(*voice).oscPitchEg);
		(*voice).egPitchModAmount = 0.5f;

		slopeEg2_init(&(*voice).oscVolEg);
		setDistortionShape(&(*voice).distortion, 0.f);

#ifdef USE_AMP_FILTER
		initOnePole(&(*voice).ampFilter);
		setOnePoleCoef(&(*voice).ampFilter,0.1f);
#endif

#if ENABLE_MIX_OSC
		(*voice).mixOscs = true;
#endif
		(*voice).decimationCnt = 0;
		(*voice).decimationRate = 1;

	
}
//---------------------------------------------------

//---------------------------------------------------
void lxr_drum_voice_trigger(DrumVoice* voice, const uint8_t vol, const uint8_t note)
{

	//update velocity modulation
	//only reset phase if envelope is closed
#ifdef USE_AMP_FILTER
	if(((*voice).volEgValueBlock[15]<=0.01f) || ((*voice).transGen.waveform==1))
#else
		//if(((*voice).ampFilterInput<=0.01f) || ((*voice).transGen.waveform==1))
#endif
	{
		float offset = 1;
		if((*voice).transGen.waveform==1) //offset mode
		{
			offset -= (*voice).transGen.volume;
#ifdef USE_AMP_FILTER
			setOnePoleCoef(&(*voice).ampFilter,1.0f); //turn off amp filter for super snappy attack

		} else {
			setOnePoleCoef(&(*voice).ampFilter,0.1f);
#endif
		}
		if((*voice).osc.waveform == SINE)
			(*voice).osc.phase = 1024 + ( (0x3ff<<20) - 1024)*offset;//(*voice).osc.startPhase ;
		else if((*voice).osc.waveform > SINE && (*voice).osc.waveform <= REC)
			(*voice).osc.phase = (0xff<<22)*offset;
		else
			(*voice).osc.phase = 0;

		//reset mod osc phase
		if((*voice).modOsc.waveform == SINE)
			(*voice).modOsc.phase = 1024 + ( (0x3ff<<20) - 1024);//(*voice).osc.startPhase ;
		else if((*voice).modOsc.waveform > SINE && (*voice).modOsc.waveform <= REC)
			(*voice).modOsc.phase = (0xff<<22);
		else
			(*voice).modOsc.phase = 0;

	}

	osc_setBaseNote(&(*voice).osc,note);
	osc_setBaseNote(&(*voice).modOsc,note);


	DecayEg_trigger(&(*voice).oscPitchEg);
	slopeEg2_trigger(&(*voice).oscVolEg);
	(*voice).velo = vol/127.f;

	transient_trigger(&(*voice).transGen);

	SnapEg_trigger(&(*voice).snapEg);

	//reset filter coeffs to prevent wrong transient
	SVF_reset(&(*voice).filter);

	}
//---------------------------------------------------
void lxr_drum_voice_update(DrumVoice* voice)
{


	//add modulation eg to osc freq (1 = no change. a+eg = original freq + modulation
	const float egPitchVal = DecayEg_calc(&(*voice).oscPitchEg);
	const float pitchEgValue = egPitchVal*(*voice).egPitchModAmount;
	(*voice).osc.pitchMod = 1+pitchEgValue;

	//calc snap EG if transient sample 0 is activated
	if((*voice).transGen.waveform == 0)
	{
		const float snapVal = SnapEg_calc(&(*voice).snapEg, (*voice).transGen.pitch);
		(*voice).osc.pitchMod += snapVal*(*voice).transGen.volume;
	}

	// fm amount with pitch eg
	(*voice).osc.fmMod = (*voice).fmModAmount * egPitchVal;

	//calc the osc + noise vol eg
#if (AMP_EG_SYNC==0)

	//check if in attack phase
	if( ((*voice).oscVolEg.attack == 1 ) && (((*voice).oscVolEg.state == EG_A) || ((*voice).oscVolEg.state == EG_REPEAT)) )
	{
			//if attack is set to 0 -> no interpolation
		(*voice).ampFilterInput = slopeEg2_calc(&(*voice).oscVolEg);
		(*voice).lastGain = (*voice).ampFilterInput;
	}
	else
	{
		(*voice).lastGain = (*voice).ampFilterInput;
		(*voice).ampFilterInput = slopeEg2_calc(&(*voice).oscVolEg);
	}

	#endif

	//update osc phaseInc
	osc_setFreq(&(*voice).osc);
	osc_setFreq(&(*voice).modOsc);



}

//---------------------------------------------------
void lxr_drum_voice_render_block(DrumVoice* voice, int16_t* buf, const uint8_t size)
{


	int16_t modBuf[size];

	//calc vol EG
#ifdef USE_AMP_FILTER
	calcOnePoleBlockFixedInput(&(*voice).ampFilter, (*voice).ampFilterInput,(*voice).volEgValueBlock, size);
#endif

	//calc next mod osc sampleBlock - int16_t processing
	calcNextOscSampleBlock(&(*voice).modOsc,modBuf,size,(*voice).fmModAmount, (*voice).pwm);

	if((*voice).mixOscs)
	{
		//calc main osc buffer - int16_t processing
		calcNextOscSampleBlock(&(*voice).osc,buf,size, (1.f-(*voice).fmModAmount), (*voice).pwm);
		//add mod buffer to main osc buffer - int16_t processing
		bufferTool_addBuffersSaturating(buf,modBuf,size);
	}
	else
	{
		calcNextOscSampleFmBlock(&(*voice).osc,modBuf,buf,size,1.0f, (*voice).pwm);
	}

	//calc transient sample - int16_t processing
	transient_calcBlock(&(*voice).transGen,modBuf,size);

	//Mix with transient buffer - int16_t processing
	bufferTool_addBuffersSaturating(buf,modBuf,size);

	//calc filter block - float processing
	SVF_calcBlockZDF(&(*voice).filter,(*voice).filterType,buf,size);

	//attentuate main OSCs by amp EG
#ifdef USE_AMP_FILTER
	bufferTool_multiplyWithFloatBufferDithered(&(*voice).dither, buf,(*voice).volEgValueBlock,size);
#else
	//bufferTool_addGainInterpolatedDoubleDithered(&(*voice).dither, buf,(*voice).ampFilterInput, (*voice).lastGain, size);
	//float
	bufferTool_addGainInterpolated(buf,(*voice).ampFilterInput, (*voice).lastGain, size);

	//buf[i] = dither_process((*voice).dither, buf[i]
#endif

	//MIDI velocity - float processing
	if((*voice).volumeMod)
	{
		bufferTool_addGain(buf,(*voice).velo,size);
	}
	//distortion - float processing
#if (USE_FILTER_DRIVE == 0)
#if SMOOTH_MIXER_VOL_PAN_DRIVE
	calcDistBlockFiltered(&(*voice).distortion,buf,size);
#else
	calcDistBlock(&(*voice).distortion,buf,size);
#endif
#endif
	//channel volume
	//bufferTool_addGain(buf,(*voice).combinedVolume  ,size);

	for(int i=0; i< size; i++)
	{
#if SMOOTH_MIXER_VOL_PAN_DRIVE

		float mixVol 	= calcOnePole( &(*voice).MixerPageFilter[0], (*voice).vol);
		(*voice).combinedVolume = mixVol * calcOnePole( &(*voice).FaderVolumeFilter, 1.0f);
#else
		(*voice).combinedVolume = (*voice).vol * calcOnePole( &(*voice).FaderVolumeFilter, 1.0f);
#endif
		buf[i] *= (*voice).combinedVolume;
	}
}
//---------------------------------------------------


