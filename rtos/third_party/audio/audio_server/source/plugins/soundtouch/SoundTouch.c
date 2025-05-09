//////////////////////////////////////////////////////////////////////////////
///
/// SoundTouch - main class for tempo/pitch/rate adjusting routines.
///
/// Notes:
/// - Initialize the SoundTouch object instance by setting up the sound stream
///   parameters with functions 'setSampleRate' and 'setChannels', then set
///   desired tempo/pitch/rate settings with the corresponding functions.
///
/// - The SoundTouch class behaves like a first-in-first-out pipeline: The
///   samples that are to be processed are fed into one of the pipe by calling
///   function 'putSamples', while the ready processed samples can be read
///   from the other end of the pipeline with function 'receiveSamples'.
///
/// - The SoundTouch processing classes require certain sized 'batches' of
///   samples in order to process the sound. For this reason the classes buffer
///   incoming samples until there are enough of samples available for
///   processing, then they carry out the processing step and consequently
///   make the processed samples available for outputting.
///
/// - For the above reason, the processing routines introduce a certain
///   'latency' between the input and output, so that the samples input to
///   SoundTouch may not be immediately available in the output, and neither
///   the amount of outputtable samples may not immediately be in direct
///   relationship with the amount of previously input samples.
///
/// - The tempo/pitch/rate control parameters can be altered during processing.
///   Please notice though that they aren't currently protected by semaphores,
///   so in multi-thread application external semaphore protection may be
///   required.
///
/// - This class utilizes classes 'TDStretch' for tempo change (without modifying
///   pitch) and 'RateTransposer' for changing the playback rate (that is, both
///   tempo and pitch in the same ratio) of the sound. The third available control
///   'pitch' (change pitch but maintain tempo) is produced by a combination of
///   combining the two other controls.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-06-13 22:29:53 +0300 (Wed, 13 Jun 2012) $
// File revision : $Revision: 4 $
//
// $Id: SoundTouch.cpp 143 2012-06-13 19:29:53Z oparviai $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include "AudioConfig.h"

#include "SoundTouch.h"
#include "TDStretch.h"
#include "RateTransposer.h"

/// test if two floating point numbers are equal
#define TEST_FLOAT_EQUAL(a, b)  (fabs(a - b) < 1e-10)

/// Calculates effective rate & tempo valuescfrom 'virtualRate', 'virtualTempo' and
/// 'virtualPitch' parameters.
void SoundTouch_calcEffectiveRateAndTempo(SoundTouch *pST);
/// Sets output pipe.
void SoundTouch_setOutPipe(SoundTouch *pST, FIFOSampleBuffer *pOutput)
{
    pST->output = pOutput;
}


void SoundTouch_init(SoundTouch *pST)
{
    // Initialize rate transposer and tempo changer instances
    memset((char *)pST, 0, sizeof(SoundTouch));
    RateTransposer_init(&(pST->rateTransposer));
    TDStretch_init(&(pST->tdStretch));
    SoundTouch_setOutPipe(pST, RateTransposer_getOutput(&(pST->rateTransposer)));
    pST->rate = pST->tempo = 0;

    pST->virtualPitch =
        pST->virtualRate =
            pST->virtualTempo = 1.0;
    SoundTouch_calcEffectiveRateAndTempo(pST);

    pST->channels = 0;
    pST->bSrateSet = RK_AUDIO_FALSE;
}



void SoundTouch_deInit(SoundTouch *pST)
{
    RateTransposer_deInit(&(pST->rateTransposer));
    TDStretch_deInit(&(pST->tdStretch));
    audio_free(mSoundTouch);
}



// Sets the number of channels, 1 = mono, 2 = stereo
void SoundTouch_setChannels(SoundTouch *pST, uint numChannels)
{

    pST->channels = numChannels;
    RateTransposer_setChannels(&(pST->rateTransposer), (int)numChannels);
    TDStretch_setChannels(&(pST->tdStretch), (int)numChannels);
}


#if SOUNDTOUCH_ABSOLUTE_VALUE
// Sets new rate control value. Normal rate = 1.0, smaller values
// represent slower rate, larger faster rates.
void SoundTouch_setRate(SoundTouch *pST, float newRate)
{
    pST->virtualRate = newRate;
    SoundTouch_calcEffectiveRateAndTempo(pST);
}
// Sets new tempo control value. Normal tempo = 1.0, smaller values
// represent slower tempo, larger faster tempo.
void SoundTouch_setTempo(SoundTouch *pST, float newTempo)
{
    pST->virtualTempo = newTempo;
    SoundTouch_calcEffectiveRateAndTempo(pST);
}
// Sets new pitch control value. Original pitch = 1.0, smaller values
// represent lower pitches, larger values higher pitch.
void SoundTouch_setPitch(SoundTouch *pST, float newPitch)
{
    pST->virtualPitch = newPitch;
    SoundTouch_calcEffectiveRateAndTempo(pST);
}
#else
// Sets new rate control value as a difference in percents compared
// to the original rate (-50 .. +100 %)
void SoundTouch_setRateChange(SoundTouch *pST, float newRate)
{
    pST->virtualRate = 1.0f + 0.01f * newRate;
    SoundTouch_calcEffectiveRateAndTempo(pST);
}
// Sets new tempo control value as a difference in percents compared
// to the original tempo (-50 .. +100 %)
void SoundTouch_setTempoChange(SoundTouch *pST, float newTempo)
{
    pST->virtualTempo = 1.0f + 0.01f * newTempo;
    SoundTouch_calcEffectiveRateAndTempo(pST);
}
// Sets pitch change in octaves compared to the original pitch
// (-1.00 .. +1.00)
void SoundTouch_setPitchOctaves(SoundTouch *pST, float newPitch)
{
    pST->virtualPitch = (float)exp(0.69314718056f * newPitch);
    SoundTouch_calcEffectiveRateAndTempo(pST);
}

// Sets pitch change in semi-tones compared to the original pitch
// (-12 .. +12)
void SoundTouch_setPitchSemiTones(SoundTouch *pST, int newPitch)
{
    SoundTouch_setPitchOctaves(pST, (float)newPitch / 12.0f);
}

void SoundTouch_setPitchSemiTones1(SoundTouch *pST, float newPitch)
{
    SoundTouch_setPitchOctaves(pST, newPitch / 12.0f);
}
#endif


// Calculates 'effective' rate and tempo values from the
// nominal control values.
void SoundTouch_calcEffectiveRateAndTempo(SoundTouch *pST)
{
    float oldTempo = pST->tempo;
    float oldRate = pST->rate;

    pST->tempo = pST->virtualTempo / pST->virtualPitch;
    pST->rate = pST->virtualPitch * pST->virtualRate;
    //DEBUG("soundTouch calcEffectiveRateAndTempo oldTempo =%f oldRate=%f tempo = %f rate=%f vTempo =%f vPitch = %f vRate=%f\n",oldTempo,oldRate, pST->tempo, pST->rate, pST->virtualTempo, pST->virtualPitch, pST->virtualRate);
    if (!TEST_FLOAT_EQUAL(pST->rate, oldRate)) RateTransposer_setRate(&(pST->rateTransposer), pST->rate);
    if (!TEST_FLOAT_EQUAL(pST->tempo, oldTempo)) TDStretch_setTempo(&(pST->tdStretch), pST->tempo);

    if (pST->rate <= 1.0f)
        SoundTouch_setOutPipe(pST, TDStretch_getOutput(&(pST->tdStretch)));
    else
        SoundTouch_setOutPipe(pST, RateTransposer_getOutput(&(pST->rateTransposer)));
}


// Sets sample rate.
void SoundTouch_setSampleRate(SoundTouch *pST, uint srate)
{
    pST->bSrateSet = RK_AUDIO_TRUE;
    // set sample rate, leave other tempo changer parameters as they are.
    TDStretch_setParameters(&(pST->tdStretch), (int)srate, -1, -1, -1);
}


// Adds 'numSamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void SoundTouch_putSamples(SoundTouch *pST, const SAMPLETYPE *samples, uint nSamples)
{

#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
    if (pST->rate <= 1.0f)
    {
        // transpose the rate down, output the transposed sound to tempo changer buffer
        //assert(output == pTDStretch);
        RateTransposer_putSamples(&(pST->rateTransposer), samples, nSamples);
        TDStretch_moveSamples(&(pST->tdStretch), (RateTransposer_getOutput(&(pST->rateTransposer))));
    }
    else
#endif
    {
        // evaluate the tempo changer, then transpose the rate up,
        //assert(output == pRateTransposer);
        TDStretch_putSamples(&(pST->tdStretch), samples, nSamples);
        RateTransposer_moveSamples(&(pST->rateTransposer), (TDStretch_getOutput(&(pST->tdStretch))));
    }
}

#if SOUNDTOUCH_NEED_FLUSH
// Flushes the last samples from the processing pipeline to the output.
// Clears also the internal processing buffers.
//
// Note: This function is meant for extracting the last samples of a sound
// stream. This function may introduce additional blank samples in the end
// of the sound stream, and thus it's not recommended to call this function
// in the middle of a sound stream.
void SoundTouch_flush(SoundTouch *pST)
{
    int i;
    int nUnprocessed;
    int nOut;
    SAMPLETYPE buff[64 * 2]; // note: allocate 2*64 to cater 64 sample frames of stereo sound

    // check how many samples still await processing, and scale
    // that by tempo & rate to get expected output sample count
    nUnprocessed = SoundTouch_numUnprocessedSamples(pST);
    nUnprocessed = (int)((double)nUnprocessed / (pST->tempo * pST->rate) + 0.5);

    nOut = FIFOSampleBuffer_numSamples(pST->output);//output->numSamples();        // ready samples currently in buffer ...
    nOut += nUnprocessed;       // ... and how many we expect there to be in the end

    memset(buff, 0, 64 * pST->channels * sizeof(SAMPLETYPE));
    // "Push" the last active samples out from the processing pipeline by
    // feeding blank samples into the processing pipeline until new,
    // processed samples appear in the output (not however, more than
    // 8ksamples in any case)
    for (i = 0; i < 128; i ++)
    {
        SoundTouch_putSamples(pST, buff, 64);
        if ((int)FIFOSampleBuffer_numSamples(pST->output) >= nOut)
        {
            // Enough new samples have appeared into the output!
            // As samples come from processing with bigger chunks, now truncate it
            // back to maximum "nOut" samples to improve duration accuracy
            FIFOSampleBuffer_adjustAmountOfSamples(pST->output, nOut);//output->adjustAmountOfSamples(nOut);

            // finish
            break;
        }
    }

    // Clear working buffers
    RateTransposer_clear(&(pST->rateTransposer));
    TDStretch_clearInput(&(pST->tdStretch));
    // yet leave the 'tempoChanger' output intouched as that's where the
    // flushed samples are!
}
#endif


// Changes a setting controlling the processing system behaviour. See the
// 'SETTING_...' defines for available setting ID's.
int SoundTouch_setSetting(SoundTouch *pST, int settingId, int value)
{
    int sampleRate, sequenceMs, seekWindowMs, overlapMs;

    // read current tdstretch routine parameters
    TDStretch_getParameters(&(pST->tdStretch), &sampleRate, &sequenceMs, &seekWindowMs, &overlapMs);

    switch (settingId)
    {
    case SETTING_SEQUENCE_MS:
        // change time-stretch sequence duration parameter
        TDStretch_setParameters(&(pST->tdStretch), sampleRate, value, seekWindowMs, overlapMs);
        return RK_AUDIO_TRUE;

    case SETTING_SEEKWINDOW_MS:
        // change time-stretch seek window length parameter
        TDStretch_setParameters(&(pST->tdStretch), sampleRate, sequenceMs, value, overlapMs);
        return RK_AUDIO_TRUE;

    case SETTING_OVERLAP_MS:
        // change time-stretch overlap length parameter
        TDStretch_setParameters(&(pST->tdStretch), sampleRate, sequenceMs, seekWindowMs, value);
        return RK_AUDIO_TRUE;

    default :
        return RK_AUDIO_FALSE;
    }
    return RK_AUDIO_FALSE;
}

#if 0
// Reads a setting controlling the processing system behaviour. See the
// 'SETTING_...' defines for available setting ID's.
//
// Returns the setting value.
int SoundTouch_getSetting(SoundTouch *pST, int settingId)
{
    int temp;

    switch (settingId)
    {

    case SETTING_SEQUENCE_MS:
        TDStretch_getParameters(&(pST->tdStretch), NULL, &temp, NULL, NULL);
        return temp;

    case SETTING_SEEKWINDOW_MS:
        TDStretch_getParameters(&(pST->tdStretch), NULL, NULL, &temp, NULL);
        return temp;

    case SETTING_OVERLAP_MS:
        TDStretch_getParameters(&(pST->tdStretch), NULL, NULL, NULL, &temp);
        return temp;

    case SETTING_NOMINAL_INPUT_SEQUENCE :
        return TDStretch_getInputSampleReq(&(pST->tdStretch));

    case SETTING_NOMINAL_OUTPUT_SEQUENCE :
        return TDStretch_getOutputBatchSize(&(pST->tdStretch));

    default :
        return 0;
    }
}
#endif

uint SoundTouch_receiveSamples(SoundTouch *pST, SAMPLETYPE *output1, uint maxSamples)
{
    return FIFOSampleBuffer_receiveSamples2(pST->output, output1, maxSamples);
};

// Clears all the samples in the object's output and internal processing
// buffers.
void SoundTouch_clear(SoundTouch *pST)
{
    RateTransposer_clear(&(pST->rateTransposer));
    TDStretch_clear(&(pST->tdStretch));
}



/// Returns number of samples currently unprocessed.
uint SoundTouch_numUnprocessedSamples(SoundTouch *pST)
{
    if (&(pST->tdStretch))
    {
        FIFOSampleBuffer *psp;

        psp = TDStretch_getInput(&(pST->tdStretch));
        if (psp)
        {
            return FIFOSampleBuffer_numSamples(psp);
        }
    }
    return 0;
}
